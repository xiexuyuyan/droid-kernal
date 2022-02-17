#include <linux/types.h>
#include <linux/ioctl.h>

/* defines a single entry that is given to a logger */
struct logger_entry {
    __u16 len; // length of payload, priority + tag + msg
    __u16 hdr_size; // sizeof(struct logger_entry_v2)
    __s32 pid; // generating process's pid
               // , tgid means thread group id
    __s32 tid; // generating process's tid
    __s32 sec; // seconds since Epoch 1970.1.1:0.0.0
    __s32 nsec; // nanoseconds since sec
    uid_t euid; //Effective UID of logger
    char msg[0]; // the entry's payload
};