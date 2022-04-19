#include "binder/ProcessState.h"

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/android/binder.h>

#include "utils/Errors.h"
#include "Static.h"
#include "binder/Parcel.h"
#include "binder/IPCThreadState.h"
#include "binder/BpBinder.h"

#undef TAG
#define TAG "ProcessState.cpp"

#define BINDER_VM_SIZE ((1 * 1024 * 1024) - sysconf(_SC_PAGE_SIZE) * 2)
#define DEFAULT_MAX_BINDER_THREADS 15

const char* defaultDriver = "/dev/binder";

namespace droid {
    sp<ProcessState> ProcessState::self() {
        Mutex::Autolock _l(gProcessMutex);
        if (gProcess != nullptr) {
            return gProcess;
        }
        gProcess = new ProcessState(defaultDriver);
        return gProcess;
    }
    sp<ProcessState> ProcessState::initWithDriver(const char *driver) {
        Mutex::Autolock _l(&gProcessMutex);
        if (gProcess != nullptr) {
            if (!strcmp(gProcess->getDriverName().c_str(), driver)) {
                return gProcess;
            }
            LOG_W(TAG, "initWithDriver: "
                       "ProcessSate was already initialized");
        }

        // todo(20220331-105620 permission!)
        if (access(driver, R_OK) == -1) {
            LOGF_E(TAG, "initWithDriver: Binder driver %s is unavailable."
                        " Using /dev/binder instead", driver);
            driver = "/dev/binder";
        }

        gProcess = new ProcessState(driver);

        return gProcess;
    }

    sp<IBinder> ProcessState::getContextObject(const sp<IBinder> &caller) {
        sp<IBinder> context = getStrongProxyForHandle(0);

        if (context == nullptr) {
            LOGF_E(TAG
            , "getContextObject: Not able to get context object on %s"
            , mDriverName.c_str());
        }

        // todo(20220325-125920 some...)

        return context;
    }

    bool ProcessState::becomeContextManager(
            context_check_func checkFunc, void *userData) {
        LOG_D(TAG, "becomeContextManager: todo");
        // todo(20220407-143512 become context manager)
        return false;
    }

    status_t ProcessState::setThreadPoolMaxThreadCount(size_t maxThreads) {
        status_t result = NO_ERROR;

        if (ioctl(mDriverFD
                  , BINDER_SET_MAX_THREADS, &maxThreads) != -1) {
            // todo(20220331-112620 impl ioctl)
            mMaxThreads = maxThreads;
        } else {
            result = -errno;
            LOGF_E(TAG, "setThreadPoolMaxThreadCount: "
                       "Binder ioctl to set max threads failed: %s"
                       , strerror(errno));
        }

        return result;
    }

    void ProcessState::setCallRestriction(
            ProcessState::CallRestriction restriction) {
        LOGF_ASSERT(IPCThreadState::selfOrNull() == nullptr
                    , "setCallRestriction: "
                      "Call restriction must set before the threadpool "
                      "is started");
        mCallRestriction = restriction;
    }

    String8 ProcessState::getDriverName() {
        return mDriverName;
    }

    static int open_driver(const char* driver) {
        int fd = open(driver, O_RDWR | O_CLOEXEC);

        int currentPid = getpid();
        LOGF_V(TAG, "open_driver: in %d, fd = %d", currentPid, fd);

        if (fd >= 0) {
            int vers = 0;
            status_t result = ioctl(fd, BINDER_VERSION, &vers);
            LOG_D(TAG, ("open_driver: ver = "
                       + std::to_string(vers)).c_str());
            if (result == -1) {
                LOG_E(TAG, ("open_driver: "
                            "Binder ioctl to obtain version failed: "
                            + std::to_string(errno)).c_str());
                close(fd);
                fd = -1;
            }
            if (result != 0 || vers != BINDER_CURRENT_PROTOCOL_VERSION) {
                LOG_E(TAG, ("open_driver: "
                            "BInder driver protocol("
                            + std::to_string(vers)
                            + ") dose not match user space protocol("
                              + std::to_string(
                                      BINDER_CURRENT_PROTOCOL_VERSION)
                              + ")! ioctl() return value: "
                              + std::to_string(result)).c_str());
                close(fd);
                fd = -1;
            }
            size_t maxThreads = DEFAULT_MAX_BINDER_THREADS;
            result = ioctl(fd, BINDER_SET_MAX_THREADS, &maxThreads);
            if (result == -1) {
                LOG_E(TAG, ("open_driver: "
                            "Binder ioctl to set max threads failed: "
                            + std::to_string(errno)).c_str());
            }
        } else {
            LOGF_E(TAG, "open_driver: open '%s' failed: %d", driver, errno);
        }

        return fd;
    }

    ProcessState::ProcessState(const char *driver) :
                mDriverName(driver)
                , mDriverFD(open_driver(driver))
                , mVMStart(MAP_FAILED)
                , mMaxThreads(DEFAULT_MAX_BINDER_THREADS)
                , mCallRestriction(CallRestriction::NONE) {
        LOGF_V(TAG, "ProcessState: constructor with driver: '%s', fd: %d"
               , mDriverName.c_str(), mDriverFD);
        if (mDriverFD >= 0) {
            mVMStart = mmap(nullptr
                    , BINDER_VM_SIZE
                    , PROT_READ, MAP_PRIVATE|MAP_NORESERVE
                    , mDriverFD, 0);
            if (mVMStart == MAP_FAILED) {
                LOG_E(TAG, ("ProcessState(): "
                            "Using " + std::string(driver) + " failed: "
                            "unable tp mmap transaction memory "
                            + std::to_string(errno)).c_str());
                close(mDriverFD);
                mDriverFD = -1;
                // todo(20220323-104311 free or clear @mDriverName
                //  , use @mDriverName instead driver above.)
            }
        }
    }

    ProcessState::~ProcessState() {
        LOG_D(TAG, "~ProcessState(): destructor");
        LOG_D(TAG, ("~ProcessState: mDriverFd = "
                    + std::to_string(mDriverFD)).c_str());
        if (mDriverFD > 0) {
            close(mDriverFD);
        }
        mDriverFD = -1;
    }


    ProcessState::handle_entry *ProcessState::lookupHandleLocked(
            int32_t handle) {
        const size_t N = mHandleToObject.size();
        LOGF_D(TAG, "lookupHandleLocked: N = %d", N);
        if (N <= (size_t)handle) {
            handle_entry entry;
            entry.binder = nullptr;
            entry.refs = nullptr;
            status_t err = mHandleToObject.insertAt(entry, N, handle + 1 - N);
            if (err < NO_ERROR)
                return nullptr;
        }
        return &mHandleToObject.editItemAt(handle);
    }

    sp<IBinder> ProcessState::getStrongProxyForHandle(int32_t handle) {
        sp<IBinder> result;

        AutoMutex  _l(mLock);

        handle_entry* entry = lookupHandleLocked(handle);

        if (entry != nullptr) {
            IBinder* proxy = entry->binder;
            if (proxy == nullptr
                || !entry->refs->attemptIncWeak(this)) {
                if (handle == 0) {
                    Parcel data;
                    status_t status = IPCThreadState::self()->transact(
                            0, IBinder::PING_TRANSACTION
                            , data, nullptr, 0);
                    if (status == DEAD_OBJECT)
                        return nullptr;
                }
                proxy = BpBinder::create(handle);
                entry->binder = proxy;
                if (proxy)
                    entry->refs = proxy->getWeakRefs();
                result = proxy;
            } else {
                LOG_E(TAG, "getStrongProxyForHandle: !!!add refs");
                // todo(20220416-144402 add refs)
            }
        }

        return result;
    }

} // namespace droid