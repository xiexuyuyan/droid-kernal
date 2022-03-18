#undef TAG
#define TAG "ProcessState.cpp"

#include "binder/ProcessState.h"

#include "Static.h"

const char* defaultDriver = "/dev/binder";

namespace droid {
    sp<ProcessState> ProcessState::self() {
        LOG_D(TAG, "self(): ");
        Mutex::Autolock _l(gProcessMutex);
        if (gProgress != nullptr) {
            return gProgress;
        }
        gProgress = new ProcessState(defaultDriver);
        return gProgress;
    }

    ProcessState::~ProcessState() {
        LOG_D(TAG, "~ProcessState(): destructor");
    }

    ProcessState::ProcessState(const char *driver) {
        LOG_D(TAG,
              std::string("ProcessState(): constructor with driver "
              + std::string(driver)).c_str());
    }

} // namespace droid