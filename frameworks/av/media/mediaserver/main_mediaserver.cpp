#include "log/log.h"
#include "utils/RefBase.h"
#include "binder/ProcessState.h"
#include "binder/IServiceManager.h"

#include <unistd.h>

#undef TAG
#define TAG "main_mediaserver.cpp"

using namespace droid;

int main() {

    LOGF_D(TAG, "main: start in %d.", getpid());

    sp<ProcessState> proc(ProcessState::self());

    sp<IServiceManager> sm = defaultServiceManager();

    return 0;
}