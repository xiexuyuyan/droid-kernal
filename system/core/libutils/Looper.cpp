#include <cstring>
#include "utils/Looper.h"
#include "utils/Mutex.h"
#include <climits>
#include <sys/eventfd.h>
#include <unistd.h>
#include "log/log.h"

#undef TAG
#define TAG "Looper.cpp"

#define DEBUG_POLL_AND_WAKE 0
#define DEBUG_CALLBACKS 0

namespace droid {
    static const int EPOLL_MAX_EVENTS = 16;
    static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
    static pthread_key_t gTLSKey = 0;


    Looper::Looper(bool allowNonCallbacks)
            : mAllowNonCallbacks(allowNonCallbacks)
            , mSendingMessage(false)
            , mPolling(false)
            , mEpollRebuildRequired(false)
            , mNextRequestSeq(0)
            , mResponseIndex(0)
            , mNextMessageUptime(LLONG_MAX) {
        // todo(20220425-095143 in android, android-base/unique_fd.h)
        mWakeEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        LOGF_ASSERT(mWakeEventFd >= 0, "Looper: Could not wake event fd: %s"
                    , strerror(errno));
        AutoMutex _l(mLock);
        rebuildEpollLocked();
    }

    void Looper::threadDestructor(void* st) {
        /*Looper*/auto* const self = static_cast<Looper*>(st);
        if (self != nullptr) {
            self->decStrong((void*) threadDestructor);
        }
    }

    void Looper::setForThread(const sp<Looper> &looper) {
        sp<Looper> old = getForThread();

        if (looper != nullptr) {
            looper->incStrong((void*)threadDestructor);
        }

        pthread_setspecific(gTLSKey, looper.get());

        if (old != nullptr) {
            old->decStrong((void*)threadDestructor);
        }
    }

    void Looper::initTLSKey() {
        int error = pthread_key_create(&gTLSKey, threadDestructor);
        LOGF_ASSERT(error == 0, "initTLSKey: Could not allocate TLS key"
                                ": %s", strerror(error));
    }

    sp<Looper> Looper::getForThread() {
        int result = pthread_once(&gTLSOnce, initTLSKey);
        LOGF_ASSERT(result == 0, "getForThread: pthread_once failed");

        return (Looper*) pthread_getspecific(gTLSKey);
    }

    sp<Looper> Looper::prepare(int opts) {
        bool allowNonCallbacks = opts & PREPARE_ALLOW_NON_CALLBACKS;
        sp<Looper> looper = Looper::getForThread();
        if (looper == nullptr) {
            looper = new Looper(allowNonCallbacks);
            Looper::setForThread(looper);
        }
        if (looper->getAllowNonCallbacks() != allowNonCallbacks) {
            LOG_W(TAG, "prepare: Looper already prepare for this thread"
                       " with a different value for the"
                       " LOOPER_PREPARE_NON_CALLBACKS options.");
        }

        return looper;
    }

    bool Looper::getAllowNonCallbacks() const {
        return mAllowNonCallbacks;
    }

    void Looper::rebuildEpollLocked() {
        if (mEpollFd > 0) {
            close(mEpollFd);
            mEpollFd = -1;
        }

        // Allocate the new epoll instance and register the wake pipe
        mEpollFd = epoll_create1(EPOLL_CLOEXEC);
        LOGF_ASSERT(mEpollFd >= 0, "rebuildEpollLocked: "
            "Could not create epoll instance: %s", strerror(errno));

        struct epoll_event wakeEventItem{};
        memset(&wakeEventItem, 0, sizeof(epoll_event));
        wakeEventItem.events = EPOLLIN;
        wakeEventItem.data.fd = mWakeEventFd;
        int result = epoll_ctl(
                mEpollFd, EPOLL_CTL_ADD, mWakeEventFd, &wakeEventItem);
        LOGF_ASSERT(result == 0, "rebuildEpollLocked: "
            "Could not add wake event fd to epoll instance: %s"
            , strerror(errno));

        for (size_t i = 0; i < mRequests.size(); i++) {
            // @mRequest is only increased after @addFd(...)
            // code-link @removeFd
            LOG_E(TAG, "rebuildEpollLocked: "
                       "mRequest increased somewhere");
            const Request& request = mRequests.valueAt(i);
            struct epoll_event eventItem{};
            request.initEventItem(&eventItem);

            int epollResult = epoll_ctl(
                    mEpollFd, EPOLL_CTL_ADD, request.fd, &eventItem);
            if (epollResult < 0) {
                LOGF_E(TAG, "rebuildEpollLocked: "
                    "Error adding epoll events for fd %d "
                    "while rebuilding epoll set: %s"
                    , request.fd, strerror(errno));
            }
        }
    }

    int Looper::pollOnce(
            int timeoutMillis
            , int *outFd
            , int *outEvents
            , void **outData) {
        int result = 0;

        for (;;) {
            while (mResponseIndex < mResponses.size()) {
                const Response& response =
                        mResponses.itemAt(mResponseIndex++);
                int ident = response.request.ident;
                if (ident >= 0) {
                    int fd = response.request.fd;
                    int events = response.events;
                    void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE
                    LOGF_D(TAG, "pollOnce: "
                        "%p ~ pollOnce - returning signalled "
                        "identifier %d: fd=%d, events=0x%x, data="
                        , this, ident, fd, events, data);
#endif // DEBUG_POLL_AND_WAKE
                    if (outFd != nullptr) *outFd = fd;
                    if (outEvents != nullptr) *outEvents = events;
                    if (outData != nullptr) *outData = data;
                    return ident;
                }
            }

            if (result != 0) {
#if DEBUG_POLL_AND_WAKE
                LOGF_D(TAG, "pollOnce: "
                            "%p ~ pollOnce - returning result %d"
                       , this, result);
#endif // DEBUG_POLL_AND_WAKE
                if (outFd != nullptr) *outFd = 0;
                if (outEvents != nullptr) *outEvents = 0;
                if (outData != nullptr) *outData = nullptr;
                return result;
            }

            result = pollInner(timeoutMillis);
        }
    }

    int Looper::pollInner(int timeoutMillis) {
#if DEBUG_POLL_AND_WAKE
        LOGF_D(TAG, "pollInner: %p ~ pollOnce - waiting: timeoutMillis=%d"
               , this, timeoutMillis);
#endif // DEBUG_POLL_AND_WAKE

        if (timeoutMillis != 0 && mNextMessageUptime != LLONG_MAX) {
            nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            int messageTimeoutMillis =
                    toMillisecondTimeoutDelay(now, mNextMessageUptime);
            if (messageTimeoutMillis >= 0
                && (timeoutMillis < 0
                    || messageTimeoutMillis < timeoutMillis)) {
                timeoutMillis = messageTimeoutMillis;
            }
        }

        int result = POLL_WAKE;
        mResponses.clear();
        mResponseIndex = 0;

        mPolling = true;

        struct epoll_event eventItems[EPOLL_MAX_EVENTS];
        int eventCount = epoll_wait(mEpollFd, eventItems
                                    , EPOLL_MAX_EVENTS, timeoutMillis);

        mPolling = false;

        mLock.lock();

        if (mEpollRebuildRequired) {
            mEpollRebuildRequired = false;
            rebuildEpollLocked();
            goto Done;
        }

        if (eventCount < 0) {
            if (errno == EINTR) {
                goto Done;
            }
            LOGF_W(TAG, "pollInner: "
                "Poll failed with an unexpected error: %s"
                , strerror(errno));
            result = POLL_ERROR;
            goto Done;
        }

        if (eventCount == 0) {
#if DEBUG_POLL_AND_WAKE
            LOGF_D(TAG, "%p ~ pollInner: timeout", this);
#endif // DEBUG_POLL_AND_WAKE
            result = POLL_TIMEOUT;
            goto Done;
        }

#if DEBUG_POLL_AND_WAKE
        LOGF_D(TAG, "%p ~ pollInner: handling events from %d fds"
               , this, eventCount);
#endif // DEBUG_POLL_AND_WAKE

        for (int i = 0; i < eventCount; i++) {
            int fd = eventItems[i].data.fd;
            uint32_t epollEvents = eventItems[i].events;
            if (fd == mWakeEventFd) {
                if (epollEvents & EPOLLIN) {
                    awoken();
                } else {
                    LOGF_W(TAG, "pollInner: "
                        "Ignoring unexpected epoll events 0x%x "
                        "on wake event fd.", epollEvents);
                }
            } else {
                ssize_t requestIndex = mRequests.indexOfKey(fd);
                if (requestIndex >= 0) {
                    int events = 0;
                    if (epollEvents & EPOLLIN) events |= EVENT_INPUT;
                    if (epollEvents & EPOLLOUT) events |= EVENT_OUTPUT;
                    if (epollEvents & EPOLLERR) events |= EVENT_ERROR;
                    if (epollEvents & EPOLLHUP) events |= EVENT_HANGUP;
                    pushResponse(events, mRequests.valueAt(requestIndex));
                } else {
                    LOGF_W(TAG, "pollInner: "
                        "Ignoring unexpected epoll events 0x%x on fd %d "
                        "that is no longer registered.", epollEvents, fd);
                }
            }
        }
Done: ;

        mNextMessageUptime = LLONG_MAX;
        while (mMessageEnvelopes.size() != 0) {
            nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            const MessageEnvelope& messageEnvelope =
                    mMessageEnvelopes.itemAt(0);
            if (messageEnvelope.uptime <= now) {
                {
                    Message message= messageEnvelope.message;
                    sp<MessageHandler> handler = messageEnvelope.handler;
                    mMessageEnvelopes.removeAt(0);
                    mSendingMessage = true;
                    mLock.unlock();
#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                    LOGF_D(TAG, "%p ~ pollInner: "
                                "sending message: handler=%p, what=%d"
                    , this, handler.get(), message.what);
#endif // DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                    handler->handleMessage(message);
                }
                mLock.lock();
                mSendingMessage = false;
                result =POLL_CALLBACK;
            } else {
                mNextMessageUptime = messageEnvelope.uptime;
                break;
            }
        }
        mLock.unlock();
        for (size_t i = 0; i < mResponses.size(); i++) {
            Response& response = mResponses.editItemAt(i);
            if (response.request.ident == POLL_CALLBACK) {
                int fd = response.request.fd;
                int events = response.events;
                void* data = response.request.data;
#if DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                LOGF_D(TAG, "%p ~ pollInner: "
                            "invoking fd event callback %p: "
                            "fd = %d, events = 0x%x, data = %p"
                , this, response.request.callback.get()
                , fd, events, data);
#endif // DEBUG_POLL_AND_WAKE || DEBUG_CALLBACKS
                int callbackResult = response.request.callback->handleEvent(fd, events, data);
                if (callbackResult == 0) {
                    // todo(20220426-103500 in app use
                    //  , it should return 0 to abort receiving callbacks)
                    // removeFd(fd, response.request.seq);
                }
                response.request.callback.clear();
            }
        }
        return result;
    }

    int Looper::pollAll(
            int timeoutMillis
            , int *outFd
            , int *outEvents
            , void **outData) {
        if (timeoutMillis <= 0) {
            int result;
            do {
                result = pollOnce(
                        timeoutMillis, outFd, outEvents, outData);
            } while (result == POLL_CALLBACK);
            return result;
        } else {
            nsecs_t endTime = systemTime(SYSTEM_TIME_MONOTONIC)
                              + milliseconds_to_nanoseconds(timeoutMillis);
            for (;;) {
                int result = pollOnce(
                        timeoutMillis, outFd, outEvents, outData);
                if (result != POLL_CALLBACK) {
                    return result;
                }
                nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
                timeoutMillis = toMillisecondTimeoutDelay(now, endTime);
                if (timeoutMillis == 0) {
                    return POLL_TIMEOUT;
                }
            }
        }
    }

    void Looper::wake() {
#if DEBUG_POLL_AND_WAKE
        LOGF_D(TAG, "%p ~ wake: ", this);
#endif // DEBUG_POLL_AND_WAKE
        uint64_t inc = 1;
        ssize_t nWrite = TEMP_FAILURE_RETRY(
                write(mWakeEventFd, &inc, sizeof(uint64_t)));
        if (nWrite != sizeof(uint64_t)) {
            if (errno != EAGAIN) {
                LOGF_E(TAG, "wake: Could not write wake signal to fd "
                            "%d (returned %zd): %s"
                            , mWakeEventFd, nWrite, strerror(errno));
            }
        }
    }

    void Looper::awoken() {
#if DEBUG_POLL_AND_WAKE
        LOGF_D(TAG, "%p ~ awoken: ", this);
#endif // DEBUG_POLL_AND_WAKE
        uint64_t counter;
        TEMP_FAILURE_RETRY(read(mWakeEventFd, &counter, sizeof(uint64_t)));
    }

    void Looper::pushResponse(int events, const Request& request) {
        Response response;
        response.events = events;
        response.request = request;
        mResponses.push(response);
    }

    void Looper::sendMessage(
            const sp<MessageHandler> &handler, const Message &message) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        sendMessageAtTime(now, handler, message);
    }

    void Looper::sendMessageDelayed(
            nsecs_t uptimeDelay
            , const sp<MessageHandler> &handler, const Message &message) {
        nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        sendMessageAtTime(now + uptimeDelay, handler, message);
    }

    void Looper::sendMessageAtTime(
            nsecs_t uptime
            , const sp<MessageHandler> &handler, const Message &message) {
#if DEBUG_CALLBACKS
        LOGF_D(TAG, "%p ~ sendMessageAtTime: "
                    "uptime=%lld, handler=%p, what=%d"
                    , this, uptime, handler.get(), message.what);
#endif // DEBUG_CALLBACKS
        size_t i = 0;
        {
            AutoMutex _l(mLock);
            size_t messageCount = mMessageEnvelopes.size();
            while (i < messageCount
                && uptime >= mMessageEnvelopes.itemAt(i).uptime) {
                i += 1;
            }
            MessageEnvelope messageEnvelope(uptime, handler, message);
            mMessageEnvelopes.insertAt(messageEnvelope, i, 1);

            if (mSendingMessage) {
                return;
            }
        }

        if (i == 0) {
            wake();
        }
    }


    void Looper::Request::initEventItem(
            struct epoll_event* eventItem) const {
        int epollEvents = 0;
        if (events & EVENT_INPUT) epollEvents |= EPOLLIN;
        if (events & EVENT_OUTPUT) epollEvents |= EPOLLOUT;

        memset(eventItem, 0, sizeof(epoll_event));
        eventItem->events = epollEvents;
        eventItem->data.fd = fd;
    }

    MessageHandler::~MessageHandler() {}

    LooperCallback::~LooperCallback() {}

} // namespace droid
