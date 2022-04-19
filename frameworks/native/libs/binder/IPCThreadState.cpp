#include "binder/IPCThreadState.h"

#include <atomic>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/android/binder.h>
#include <binder/Binder.h>

#include "log/log.h"

#undef TAG
#define TAG "IPCThreadState.cpp"

namespace droid {
    static pthread_mutex_t gTLSMutex = PTHREAD_MUTEX_INITIALIZER;
    static std::atomic<bool> gHaveTLS(false);
    static pthread_key_t gTLS = 0;
    static std::atomic<bool> gShutdown(false);
    // todo(20220330-111301 why android source use implicit constructor = bool)


    IPCThreadState *droid::IPCThreadState::self() {
        if (gHaveTLS.load(std::memory_order_acquire)) {
restart:
            const pthread_key_t key = gTLS;
            IPCThreadState* state =
                    (IPCThreadState*) pthread_getspecific(key);
            if (state)
                return state;
            return new IPCThreadState;
        }

        if (gShutdown.load(std::memory_order_relaxed)) {
            LOG_W(TAG, "self: Calling during shutdown");
            return nullptr;
        }

        pthread_mutex_lock(&gTLSMutex);
        if (!gHaveTLS.load(std::memory_order_relaxed)) {
            int key_create_value = pthread_key_create(
                    &gTLS, threadDestructor);
            if (key_create_value != 0) {
                pthread_mutex_unlock(&gTLSMutex);
                LOGF_E(TAG, "self: Unable to create TLS key"
                            ", expect a crash %d", key_create_value);
                return nullptr;
            }
            gHaveTLS.store(true, std::memory_order_release);
        }
        pthread_mutex_unlock(&gTLSMutex);
        goto restart;
    }

    IPCThreadState *IPCThreadState::selfOrNull() {
        if (gHaveTLS.load(std::memory_order_acquire)) {
            const pthread_key_t key = gTLS;
            /*IPCThreadState*/auto * state =
                    (IPCThreadState*) pthread_getspecific(key);
            return state;
        }
        return nullptr;
    }


    void IPCThreadState::clearCaller() {
        mCallingPid = getpid();
        mCallingSid = nullptr;
        mCallingUid = getuid();
    }

    void IPCThreadState::flushCommands() {
        // todo(20220330-145853 to impl this
        //  , it called by @threadDestructor())
    }

    status_t IPCThreadState::transact(
            int32_t handle, uint32_t code
            , const Parcel &data, Parcel *reply
            , uint32_t flags) {
        // todo(20220330-145755 to complete IPCThread's transact)
        status_t err;

        flags |= TF_ACCEPT_FDS;

        LOGF_V(TAG, "transact: BC_TRANSACTION "
                    "tid(%d-%lu) handle(%d) code(%d)"
               , gettid(), pthread_self(), handle, code);

        LOGF_V(TAG, "transact: Send from pid(%d) uid(%d) %s"
              , getpid(), getuid()
              , (flags & TF_ONE_WAY) ? "READ REPLY" : "ONE WAY");
        err = writeTransactionData(
                BC_TRANSACTION, flags, handle, code, data, nullptr);

        if (err != NO_ERROR) {
            if (reply)
                reply->setError(err);
            return (mLastError = err);
        }

        if ((flags & TF_ONE_WAY) == 0) {
            // todo(20220418-191606 non-oneway restriction)

            if (reply) {
                err = waitForResponse(reply);
            } else {
                Parcel fakeReply;
                err = waitForResponse(&fakeReply);
            }

            LOGF_V(TAG, "transact: BR_REPLY "
                        "tid(%d-%lu) handle(%d) code(%d)"
                        , gettid(), pthread_self(), handle, code);
        } else {
            err = waitForResponse(nullptr, nullptr);
        }

        return err;
    }

    IPCThreadState::IPCThreadState()
        : mProcess(ProcessState::self()) {
        pthread_setspecific(gTLS, this);
        clearCaller();
        // todo(20220330-145821 to complete IPCThreadState's constructor)
        LOGF_D(TAG
               , "IPCThreadState: constructor pid = %d, uid = %d"
               , mCallingPid, mCallingUid);
    }

    IPCThreadState::~IPCThreadState() {
        LOG_D(TAG, "~IPCThreadState: destructor");
    }

    void IPCThreadState::threadDestructor(void *state) {
        LOG_D(TAG, "threadDestructor: ");
        // todo(20220330-162320 why main process exit, it doesn't reach here)
        /*IPCThreadState*/auto* const self = static_cast<IPCThreadState*>(state);
        if (self) {
            self->flushCommands();
            if (self->mProcess->mDriverFD >= 0) {
                ioctl(self->mProcess->mDriverFD
                      , BINDER_THREAD_EXIT, 0);
                // todo(20220330-110428 impl ioctl BINDER_THREAD_EXIT in driver)
            }
            delete self;
        }
    }

    sp<BBinder> the_context_object;

    void IPCThreadState::setTheContextObject(sp<BBinder> obj) {
        the_context_object = obj;
        LOG_D(TAG, "setTheContextObject: ");
    }

    status_t IPCThreadState::waitForResponse(
            Parcel *reply, status_t *acquireResult) {
        uint32_t cmd;
        int32_t err;
        while (1) {
            if ((err = talkWithDriver()) < NO_ERROR) {
                break;
            }

            err = mIn.errorCheck();
            if (err < NO_ERROR) {
                break;
            }

            // cmd = (uint32_t)mIn.readInt32();
            LOGF_D(TAG, "waitForResponse: %d", cmd);
            goto finish;
        }

finish:
        if (err != NO_ERROR) {
            if (acquireResult) {
                *acquireResult = err;
            }
            if (reply) {
                reply->setError(err);
            }
            mLastError = err;
        }

        return err;
    }

    status_t IPCThreadState::talkWithDriver(bool doReceive) {
        if (mProcess->mDriverFD < 0) {
            LOGF_E(TAG, "talkWithDriver: fd(%d)", mProcess->mDriverFD);
            return -EBADF;
        }

        binder_write_read bwr;

        // is the read buffer empty?
        const bool needRead = mIn.dataPosition() >= mIn.dataSize();
        // todo(20220419-101208 we assume that
        //  @mDataPos is always <= @mDataSize )
        LOGF_D(TAG, "talkWithDriver: "
                    "mIn.dataPosition() = %d, mIn.dataSize() = %d"
                    , mIn.dataPosition(), mIn.dataSize());

        /* @doReceive is default set to true
         * , only receive return-cmds from driver
         * -----------------------------
           | doReceive | needRead |
           | True      | True     | data in mIn from driver finish handle
           | True      | False    | data in mIn from driver not ...
           ------------------------------
         */
        const size_t outAvail =
                (!doReceive || needRead) ? mOut.dataSize() : 0;
        LOGF_D(TAG, "talkWithDriver: outAvail(%d)", outAvail);
        bwr.write_size = outAvail;
        bwr.write_buffer = (uintptr_t)mOut.data();

        if (doReceive && needRead) {
            bwr.read_size = mIn.dataCapacity();
            bwr.read_buffer = (uintptr_t)mIn.data();
        } else {
            bwr.read_size = 0;
            bwr.read_buffer = 0;
        }

        if ((bwr.write_size == 0) && (bwr.read_size == 0)) {
            return NO_ERROR;
        }

        bwr.write_consumed = 0;
        bwr.read_consumed = 0;
        status_t err;
        do {
            LOGF_D(TAG, "talkWithDriver: "
                        "about to read/write, write size = %d"
                        , mOut.dataSize());
            if (ioctl(mProcess->mDriverFD, BINDER_WRITE_READ, &bwr)) {
                err = NO_ERROR;
            } else {
                err = -errno;
            }
            LOGF_D(TAG, "talkWithDriver: err = %s", strerror(err));
        } while(err == -EINTR);

        return -EBADF;;
    }

    status_t IPCThreadState::writeTransactionData(
            int32_t cmd
            , uint32_t binderFlags
            , int32_t handle
            , uint32_t code
            , const Parcel &data
            , status_t *statusBuffer) {
        binder_transaction_data tr;

        tr.target.ptr = 0;
        tr.target.handle = handle;
        tr.code = code;
        tr.flags = binderFlags;
        tr.cookie = 0;
        tr.sender_pid = 0;
        tr.sender_euid = 0;

        const status_t err = data.errorCheck();
        LOGF_D(TAG, "writeTransactionData: cmd = %x", cmd);
        if (err == NO_ERROR) {
            tr.data_size = data.ipcDataSize();
            tr.data.ptr.buffer = data.ipcData();
            tr.offsets_size = data.ipcObjectsCount()*sizeof(binder_size_t);
            tr.data.ptr.offsets = data.ipcObjects();
        } else if (statusBuffer) {
            // todo(20220418-102037 when it goto this condition?)
            tr.flags |= TF_STATUS_CODE;
            *statusBuffer = err;
            tr.data_size = sizeof(status_t);
            tr.data.ptr.buffer =
                    reinterpret_cast<uintptr_t>(statusBuffer);
            tr.offsets_size = 0;
            tr.data.ptr.offsets = 0;
        } else {
            // todo(20220418-102308 and this condition?
            //  error happened and write buffer is nullptr)
            LOG_E(TAG, "writeTransactionData: "
                       "error happened and write buffer is nullptr");
            return (mLastError = err);
        }

        mOut.writeInt32(cmd);
        mOut.write(&tr, sizeof(tr));

        return NO_ERROR;
    }
}