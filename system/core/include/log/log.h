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
    DROID_LOG_UNKNOWN = 0,
    DROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    DROID_LOG_VERBOSE,
    DROID_LOG_DEBUG,
    DROID_LOG_INFO,
    DROID_LOG_WARN,
    DROID_LOG_ERROR,
    DROID_LOG_FATAL,
    DROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
};

#define LOGGER_LOG_RADIO  "log_radio"
#define LOGGER_LOG_EVENTS "log_events"
#define LOGGER_LOG_SYSTEM "log_system"
#define LOGGER_LOG_MAIN   "log_main"

#define LOGGER_MAX_LENGTH (4*1024)

int __droid_log_buf_write(int bufID, int prio, const char *tag, const char *msg);
int __droid_log_buf_write_f(
        int bufID, int priority, const char* tag
        , const char* format, ...);

#define LOG_I(TAG, MSG) __droid_log_buf_write(LOG_ID_MAIN, DROID_LOG_INFO, (TAG), (MSG))
#define LOG_D(TAG, MSG) __droid_log_buf_write(LOG_ID_MAIN, DROID_LOG_DEBUG, (TAG), (MSG))
#define LOG_E(TAG, MSG) __droid_log_buf_write(LOG_ID_MAIN, DROID_LOG_ERROR, (TAG), (MSG))

#define LOGF_I(TAG, FORMAT, ...) __droid_log_buf_write_f(LOG_ID_MAIN, DROID_LOG_INFO, TAG, FORMAT, ##__VA_ARGS__)
#define LOGF_D(TAG, FORMAT, ...) __droid_log_buf_write_f(LOG_ID_MAIN, DROID_LOG_DEBUG, TAG, FORMAT, ##__VA_ARGS__)
#define LOGF_E(TAG, FORMAT, ...) __droid_log_buf_write_f(LOG_ID_MAIN, DROID_LOG_ERROR, TAG, FORMAT, ##__VA_ARGS__)

#endif // LIB_LOG_LOG_H
