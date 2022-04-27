#ifndef UTILS_LOOPER_H
#define UTILS_LOOPER_H

#include "RefBase.h"
#include <sys/epoll.h>
#include "utils/KeyedVector.h"
#include "utils/Timers.h"
#include "Mutex.h"

namespace droid {

    struct Message {
        Message() : what(0) {}
        Message(int w) : what(w) {}
        int what;
    };

    class MessageHandler : public virtual RefBase {
    protected:
        virtual ~MessageHandler();
    public:
        virtual void handleMessage(const Message& message) = 0;
    };


    class LooperCallback : public virtual RefBase {
    protected:
        virtual ~LooperCallback();
    public:
        virtual int handleEvent(int fd, int events, void* data) = 0;
    };



    class Looper : public RefBase {
    public:
        enum {
            POLL_WAKE     = -1,
            POLL_CALLBACK = -2,
            POLL_TIMEOUT  = -3,
            POLL_ERROR    = -4
        };

        enum {
            EVENT_INPUT   = 1 << 0,
            EVENT_OUTPUT  = 1 << 1,
            EVENT_ERROR   = 1 << 2,
            EVENT_HANGUP  = 1 << 3,
            EVENT_INVALID = 1 << 4,
        };

        enum {
            PREPARE_ALLOW_NON_CALLBACKS = 1 << 0,
        };
        explicit Looper(bool allowNonCallbacks);

        bool getAllowNonCallbacks() const;

        int pollOnce(
                int timeoutMillis
                , int* outFd
                , int* outEvents
                , void** outData);
        inline int pollOnce(int timeoutMillis) {
            return pollOnce(timeoutMillis, nullptr, nullptr, nullptr);
        }

        int pollAll(
                int timeoutMillis
                , int* outFd
                , int* outEvents
                , void** outData);
        inline int pollAll(int timeoutMillis) {
            return pollAll(timeoutMillis, nullptr, nullptr, nullptr);
        }

        void wake();


        void sendMessage(
                const sp<MessageHandler>& handler, const Message& message);
        void sendMessageDelayed(
                nsecs_t uptimeDelay
                , const sp<MessageHandler>& handler, const Message& message);
        void sendMessageAtTime(
                nsecs_t uptime
                , const sp<MessageHandler>& handler, const Message& message);

        static sp<Looper> prepare(int opts);

        static void setForThread(const sp<Looper>& looper);

        static sp<Looper> getForThread();

    private:
        struct Request {
            int fd;
            int ident;
            int events;
            int seq;
            sp<LooperCallback> callback;
            void* data;

            void initEventItem(struct epoll_event* eventItem) const;
        };
        struct Response {
            int events;
            Request request;
        };

        struct MessageEnvelope {
            MessageEnvelope() : uptime(0) {}

            MessageEnvelope(
                    nsecs_t u
                    , const sp<MessageHandler> h
                    , const Message& m)
                    : uptime(u), handler(h), message(m) {
            }

            nsecs_t uptime;
            sp<MessageHandler> handler;
            Message message;
        };

        const bool mAllowNonCallbacks;

        int        mWakeEventFd = -1;
        Mutex      mLock;

        Vector<MessageEnvelope> mMessageEnvelopes;
        bool mSendingMessage;

        volatile bool mPolling;

        int        mEpollFd = -1;
        bool       mEpollRebuildRequired;

        KeyedVector<int, Request> mRequests;
        int        mNextRequestSeq;

        // This state is only used privately by @pollOnce
        // and does not require a lock since it runs
        // on a single thread
        Vector<Response> mResponses;
        size_t           mResponseIndex;
        nsecs_t          mNextMessageUptime;

        int              pollInner(int timeoutMillis);
        void             awoken();
        void             pushResponse(int events, const Request& request);
        void             rebuildEpollLocked();

        static void initTLSKey();
        static void threadDestructor(void* st);
    };
} // namespace droid






#endif // UTILS_LOOPER_H
