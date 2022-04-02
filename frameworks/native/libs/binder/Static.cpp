#include "Static.h"

namespace droid {
    // ProcessState.cpp
    Mutex& gProcessMutex = *new Mutex;
    sp<ProcessState> gProcess;
} // namespace droid