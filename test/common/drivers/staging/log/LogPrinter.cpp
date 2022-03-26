#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cstring>

#include "logger.h"

enum LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
};
static char LogPriorityCharacter[] {
    /* ANDROID_LOG_UNKNOWN */ 'U',
    /* ANDROID_LOG_DEFAULT */ 'D',    /* only for SetMinPriority() */
    /* ANDROID_LOG_VERBOSE */ 'V',
    /* ANDROID_LOG_DEBUG   */ 'D',
    /* ANDROID_LOG_INFO    */ 'I',
    /* ANDROID_LOG_WARN    */ 'W',
    /* ANDROID_LOG_ERROR   */ 'E',
    /* ANDROID_LOG_FATAL   */ 'F',
    /* ANDROID_LOG_SILENT  */ 'S',     /* only for SetMinPriority(); must be last */
};


int main(int argc, char* argv[]) {
    std::cout<<"---start print log---"<<std::endl;

    for (int i = 1; i < argc; ++i) {
        std::cout<<"argv["<<i;
        std::cout<<"] = "<<argv[i]<<std::endl;
    }

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

            char* tag = (char*)(entry->msg + 1);
            char* msg = (char*)(tag + strlen(tag) + 1);
            char priority = *(char*)entry->msg;


            bool mskip = false;
            for (int i = 1; i < argc; ++i) {
                if (!strcmp(tag, argv[i])) {
                    mskip = true;
                    break;
                }
            }
            if (mskip) {
                continue;
            }

            {
                std::cout<<entry->pid<<" ";
                std::cout<<entry->tid<<" ";
            }
            std::cout<<LogPriorityCharacter[priority - '0'];
            std::cout<<"/"<<tag;
            std::cout<<" "<<msg<<std::endl;
        }
    }
}