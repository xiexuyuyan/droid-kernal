#ifndef LIB_LOG_LOG_H
#define LIB_LOG_LOG_H

#include "iostream"

enum LogId{
    LOG_ID_MAIN   = 0,
    LOG_ID_RADIO  = 1,
    LOG_ID_EVENTS = 2,
    LOG_ID_SYSTEM = 3,
    LOG_ID_CRASH  = 4,
};

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

#define LOGGER_LOG_RADIO  "log_radio"
#define LOGGER_LOG_EVENTS "log_events"
#define LOGGER_LOG_SYSTEM "log_system"
#define LOGGER_LOG_MAIN   "log_main"

int __droid_log_buf_write(int bufID, int prio, const char *tag, const char *msg);


#endif // LIB_LOG_LOG_H
