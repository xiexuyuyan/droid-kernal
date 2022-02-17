#undef LOG_TAG
#define LOG_TAG "OpenLogDriver.cpp"

#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "log/log.h"
#include "logger.h"


#define LOGGER_LOG_MAIN   "/dev/log_main"


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

    if (argc == 2) {
        write(fd, argv[1], strlen(argv[1]));
    } else {
        int lenHeader = sizeof(struct logger_entry);
        char *readBuf = static_cast<char *>(malloc(lenHeader));
        read(fd, readBuf, lenHeader);
        struct logger_entry *entry = reinterpret_cast<logger_entry *>(readBuf);
        LOG_D(LOG_TAG, "main(): entry->len = " + std::to_string(entry->len));
    }

    close(fd);

    return 0;
}