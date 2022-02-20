#undef LOG_TAG
#define LOG_TAG "OpenLogDriver.cpp"

#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>

#include "logger.h"

void LOG_D(const std::string& TAG, const std::string& text) {
    std::cout<<TAG<<": "<<text<<std::endl;
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

    if (argc == 2) {
        write(fd, argv[1], strlen(argv[1]));
    } else {
        struct logger_entry *entry;

        entry = static_cast<logger_entry *>(malloc(LOGGER_ENTRY_MAX_LEN));

        while (true) {
            memset(entry, 0, LOGGER_ENTRY_MAX_LEN);
            read(fd, entry, LOGGER_ENTRY_MAX_LEN);
            LOG_D(LOG_TAG, "main(): entry->len = " + std::to_string(entry->len));
            LOG_D(LOG_TAG, "main: entry->msg = " + std::string(entry->msg));
        }

        free(entry);
    }

    close(fd);

    return 0;
}