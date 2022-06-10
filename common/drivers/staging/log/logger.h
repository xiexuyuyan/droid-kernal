#ifndef DRIVER_LOG_LOGGER_H
#define DRIVER_LOG_LOGGER_H

#include <linux/types.h>
#include <linux/ioctl.h>

/* defines a single entry that is given to a logger */
struct logger_entry {
    __u16 len; // length of payload, priority + tag + msg
    __u16 hdr_size; // sizeof(struct logger_entry_v2)
    __s32 pid; // generating process's pid
               // , tgid means thread group id
    __s32 tid; // generating process's tid
    __s32 sec; // seconds since boot
    __s32 nsec; // nanoseconds since sec
    __s32 real_sec; // seconds since utc
    __s32 real_nsec; // nanoseconds since sec
    kuid_t euid; //Effective UID of logger
    char msg[0]; // the entry's payload
};


#define LOGGER_LOG_RADIO  "log_radio"
#define LOGGER_LOG_EVENTS "log_events"
#define LOGGER_LOG_SYSTEM "log_system"
#define LOGGER_LOG_MAIN   "log_main"

#define LOGGER_ENTRY_MAX_PAYLOAD (4*1024)

#endif // DRIVER_LOG_LOGGER_H