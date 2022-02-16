#undef LOG_TAG
#define LOG_TAG "OpenLogDriver.cpp"


#include <fcntl.h>
#include <unistd.h>

#include "log/log.h"


#define LOGGER_LOG_MAIN   "/dev/log_main"

int main() {
    LOG_D(LOG_TAG, "main: start");

    int fd;

    fd = open(LOGGER_LOG_MAIN, O_RDWR);
    if (fd < 0) {
        LOG_D(LOG_TAG, "main: open "
                        + std::string(LOGGER_LOG_MAIN)
                        + " failed!");
        return -1;
    } else {
        LOG_D(LOG_TAG, "main: fd = " + std::to_string(fd));
    }

    close(fd);

    return 0;
}