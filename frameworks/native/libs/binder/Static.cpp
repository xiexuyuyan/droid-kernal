#include "Static.h"

namespace droid {
    // ProcessState.cpp
    Mutex& gProcessMutex = *new Mutex;
    sp<ProcessState> gProgress;
} // namespace droid