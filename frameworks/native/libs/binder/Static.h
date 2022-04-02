#include "binder/ProcessState.h"

namespace droid {
    extern Mutex& gProcessMutex;
    extern sp<ProcessState> gProcess;
}