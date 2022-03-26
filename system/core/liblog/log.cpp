#include <sys/uio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "log/log.h"


static int write_to_log_init(LogId logId, struct iovec *vec, size_t nr);
static int __write_to_log(LogId logId, struct iovec *vec, size_t nr);

static int (*write_to_log)(LogId logId, struct iovec *vec, size_t nr) = write_to_log_init;
static int logFd[4];

int __droid_log_buf_write(int bufID, int priority, const char *tag, const char *msg) {
    struct iovec vec[3];

    if (!tag)
        return 0;

    // format ascii int -> char
    priority += '0';

    vec[0].iov_base = (unsigned char *) &priority;
    vec[0].iov_len = 1;
    vec[1].iov_base = (void *)tag;
    vec[1].iov_len = strlen(tag) + 1;
    vec[2].iov_base = (void *)msg;
    vec[2].iov_len = strlen(msg) + 1;

    return write_to_log(static_cast<LogId>(bufID), vec, 3);
}

int __droid_log_buf_write_f(
        int bufID, int priority, const char* tag
        , const char* format, ...) {
    char buf[LOGGER_MAX_LENGTH] = {0};
    va_list p;

    va_start(p, format);
    vsprintf(buf, format, p);
    va_end(p);

    return __droid_log_buf_write(bufID, priority, tag, buf);
}

static int write_to_log_init(LogId logId, struct iovec *vec, size_t nr) {
    if (write_to_log == write_to_log_init) {
        logFd[LOG_ID_MAIN] = open("/dev/" LOGGER_LOG_MAIN, O_WRONLY);
        logFd[LOG_ID_RADIO] = open("/dev/" LOGGER_LOG_RADIO, O_WRONLY);
        logFd[LOG_ID_EVENTS] = open("/dev/" LOGGER_LOG_EVENTS, O_WRONLY);
        logFd[LOG_ID_SYSTEM] = open("/dev/" LOGGER_LOG_SYSTEM, O_WRONLY);

        if (
                logFd[LOG_ID_MAIN] < 0 ||
                logFd[LOG_ID_RADIO] < 0 ||
                logFd[LOG_ID_EVENTS] < 0 ||
                logFd[LOG_ID_SYSTEM] < 0
        ) {
            close(logFd[LOG_ID_MAIN]);
            close(logFd[LOG_ID_RADIO]);
            close(logFd[LOG_ID_EVENTS]);
            close(logFd[LOG_ID_SYSTEM]);

            logFd[LOG_ID_MAIN] = -1;
            logFd[LOG_ID_RADIO] = -1;
            logFd[LOG_ID_EVENTS] = -1;
            logFd[LOG_ID_SYSTEM] = -1;

            std::cout<<"__write_to_log_init open fd failed!"<<std::endl;
            return -1;
        } else {
            write_to_log = __write_to_log;
        }
    }

    return write_to_log(logId, vec, nr);
}

static int __write_to_log(LogId logId, struct iovec *vec, size_t nr) {
    int ret;
/*
    std::cout<<"---------- __write_to_log ----------"<<std::endl;
    std::cout<<"logId = "<<logId<<", fd = "<<logFd[logId]<<std::endl;
    std::cout<<"vec[0]: len = "<<vec[0].iov_len<<std::endl;
    std::cout<<"vec[0]: base = "<<(char*)vec[0].iov_base<<std::endl;
    std::cout<<"vec[1]: len = "<<vec[1].iov_len<<std::endl;
    std::cout<<"vec[1]: base = "<<(char*)vec[1].iov_base<<std::endl;
    std::cout<<"vec[2]: len = "<<vec[2].iov_len<<std::endl;
    std::cout<<"vec[2]: base = "<<(char*)vec[2].iov_base<<std::endl;
    std::cout<<"---------- __write_to_log ----------"<<std::endl;
*/

    // ret = writev(STDOUT_FILENO, vec, nr);
    ret = writev(logFd[logId], vec, nr);

    return ret;
}