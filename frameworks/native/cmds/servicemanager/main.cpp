#include "log/log.h"
#include "binder/ProcessState.h"
#include "binder/IPCThreadState.h"
#include "utils/StrongPointer.h"
#include "utils/Looper.h"
#include "droid/os/IServiceManager.h"

#include "ServiceManager.h"

#undef TAG
#define TAG "main.cpp"

using ::droid::sp;
using ::droid::wp;
using ::droid::Looper;
using ::droid::ProcessState;
using ::droid::IPCThreadState;
using ::droid::ServiceManager;
using ::droid::os::IServiceManager;

int main(int argc, char* argv[]) {
    LOG_D(TAG, "main: start");

    const char* driver = "/dev/binder";

    sp<ProcessState> ps = ProcessState::initWithDriver(driver);
    ps->setThreadPoolMaxThreadCount(0);
    ps->setCallRestriction(
            ProcessState::CallRestriction::FATAL_IF_NOT_ONEWAY);

    sp<ServiceManager> manager = new ServiceManager;
    if (!manager->addService(
            "manager", manager, false
            , IServiceManager::DUMP_FLAG_PRIORITY_DEFAULT).isOk()) {
        LOG_E(TAG, "main: Could not self register servicemanager");
    }
    IPCThreadState::self()->setTheContextObject(manager);
    ps->becomeContextManager(nullptr, nullptr);

    sp<Looper> looper = Looper::prepare(false /*allowNonCallbacks*/);

    LOG_D(TAG, "main: end\n\n\n");
    return 0;
}