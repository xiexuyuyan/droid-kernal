#include "log/log.h"
#include "utils/RefBase.h"
#include "binder/ProcessState.h"

#include <unistd.h>

#undef TAG
#define TAG "main_mediaserver.cpp"

using namespace droid;

int main() {
    int currentPid = getpid();

    LOG_D("main", ("start in media server! in pid: " + std::to_string(currentPid)).c_str());

    sp<ProcessState> proc(ProcessState::self());

    return 0;
}