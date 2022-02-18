#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>

#include "logger.h"

int main(int argc, char* argv[]) {
    std::cout<<"---start print log---"<<std::endl;

    int fd;
    fd = open(LOGGER_LOG_MAIN, O_RDWR);
    if (fd < 0) {
        std::cout<<"open "<<LOGGER_LOG_MAIN<<" failed!"<<std::endl;
        return 0;
    }
    {
        struct logger_entry* entry;
        entry = static_cast<struct logger_entry*>(malloc(LOGGER_ENTRY_MAX_LEN));

        while (true) {
            read(fd, entry, LOGGER_ENTRY_MAX_LEN);
            std::cout << entry->msg << std::endl;
        }
    }
}