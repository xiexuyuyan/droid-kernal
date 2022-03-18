#include "log/log.h"
#include "utils/RefBase.h"
#include "binder/ProcessState.h"

#undef TAG
#define TAG "main_mediaserver.cpp"

using namespace droid;

int main() {
    LOG_D("main", "start in media server!");

    sp<ProcessState> proc(ProcessState::self());

    return 0;
}