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

    int keyWordStart = 3;

    int withTimestamp = 3;
    if (argc > 1) {
        withTimestamp = argv[1][0] - '0';
    }

    int withProcess = 1;
    if (argc > 2) {
        withProcess = argv[2][0] - '0';
    }

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
            for (int i = keyWordStart; i < argc; ++i) {
                if (!strcmp(tag, argv[i])) {
                    mskip = true;
                    break;
                }
            }
            if (mskip) {
                continue;
            }

            if (1 == withProcess) {
                std::cout<<entry->pid<<" ";
                std::cout<<entry->tid<<" ";
            }

            char *timeFormat = NULL;
            switch (withTimestamp) {
                case 0:
                    break;
                case 3:
                    std::cout<<entry->sec<<".";
                    std::cout<<(entry->nsec / 100)<<" ";
                case 2:
                    timeFormat = "%Y-%m-%d %H:%M:%S";
                case 1:
                    timeFormat = const_cast<char *>(
                            (timeFormat == NULL) ? "%H:%M:%S" : timeFormat);
                    time_t t = entry->real_sec;
                    struct tm* time = localtime(&t);
                    char nowStr[64];
                    strftime(nowStr, 64, timeFormat, time);
                    std::cout<<nowStr<<".";
                    std::cout<<(entry->real_nsec / 1000000)<<" ";
                    break;
            }

            std::cout<<LogPriorityCharacter[priority - '0'];
            std::cout<<"/"<<tag;
            std::cout<<" "<<msg<<std::endl;
        }
    }
}