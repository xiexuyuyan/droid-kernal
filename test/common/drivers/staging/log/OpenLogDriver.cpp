#undef LOG_TAG
#define LOG_TAG "OpenLogDriver.cpp"

#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "log/log.h"


#define LOGGER_LOG_MAIN   "/dev/log_main"

static void sample_test() {
    size_t a = 1024;
    size_t i = 0;
    for (; i < 10; ++i) {
        size_t b = (a - 1) & i;
        LOG_D(LOG_TAG, "sample_test: " + std::to_string(b));
    }
}

int main(int argc, char* argv[]) {
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

    char a[] = "hello world";
    if (argc == 2) {
        write(fd, argv[1], strlen(argv[1]));
    } else {
        write(fd, a, strlen(a));
    }

    close(fd);

    return 0;
}