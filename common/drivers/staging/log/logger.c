#define pr_fmt(fmt) "logger: " fmt

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <linux/uio.h>
#include "logger.h"

#include <asm/ioctls.h>

#include <linux/sched/signal.h>

/*----------------------------------------------------------------------*/
#ifndef __KERNEL__
#define likely(x) ((x) != 0)
#define unlikely(x) ((x) != 0)
#endif // __KERNEL__
/*----------------------------------------------------------------------*/


/* represent a specific log, such as main or radio */
struct logger_log {
    unsigned char* buffer; // the actual ring buffer
    struct miscdevice misc; // device that representing the log
    wait_queue_head_t wq; // wait queue for @readers
    struct list_head readers; // This log's readers
    struct mutex       mutex; // the mutex protects the @buffer
    size_t             w_off; // the current write head offset
    size_t              head; // the head, or location start reading
    size_t              size; // the size of the log
                              // , assign in create_log(, @size)
    struct list_head    logs; // the list of log channels
};

/* a logging device open for reading */
struct logger_reader {
    struct logger_log* log; // associated log
    struct list_head list; // entry in @logger_log's list @readers
    size_t    r_off; // current read head offset
    bool      r_all; // reader can read all entries
    int       r_ver; // reader api version
};

static LIST_HEAD(log_list);

static size_t logger_offset(struct logger_log* log, size_t n) {
    return n & (log->size - 1);
}

static size_t get_next_entry_by_uid(struct logger_log* log
        , size_t off, kuid_t euid) {
    // TODO("permission about, trigger when can't read all log buffer")
    return 0;
}

static size_t get_user_hdr_len(int ver) {
    return sizeof(struct logger_entry);
}

static struct logger_entry* get_entry_header(
        struct logger_log* log, size_t off
        , struct logger_entry* scratch) {
    size_t len = min(sizeof(struct logger_entry), log->size - off);

    if (len != sizeof(struct logger_entry)) {
        memcpy(((void *)scratch), log->buffer + off, len);
        memcpy(((void *)scratch) + len, log->buffer
                , sizeof(struct logger_entry) - len);

        return scratch;
    }

    return (struct logger_entry*)(log->buffer + off);
}

static __u32 get_entry_msg_len(struct logger_log* log
        , size_t off) {
    struct logger_entry scratch;
    struct logger_entry* entry;

    entry = get_entry_header(log, off, &scratch);
    return entry->len;
}

static ssize_t copy_header_to_user(int ver
         , struct logger_entry* entry, char __user* buf) {
    void* hdr;
    size_t hdr_len;

    hdr = entry;
    hdr_len = sizeof(struct logger_entry);

    return copy_to_user(buf, hdr, hdr_len);
}

static ssize_t do_read_log_to_user(
        struct logger_log* log
        , struct logger_reader* reader
        , char __user* buf, size_t count) {
    // count = header + log->len
    struct logger_entry scratch;
    struct logger_entry* entry;
    size_t len;
    size_t msg_start;

    entry = get_entry_header(log, reader->r_off, &scratch);
    if (copy_header_to_user(reader->r_ver, entry, buf))
        return -EFAULT;

    count -= get_user_hdr_len(reader->r_ver);
    buf += get_user_hdr_len(reader->r_ver);
    msg_start = logger_offset(log
            , reader->r_off + sizeof(struct logger_entry));

    // now count only remains length of payload
    len = min(count, log->size - msg_start);
    if (copy_to_user(buf, log->buffer + msg_start, len))
        return -EFAULT;

    if (count != len)
        // if here, means len = some of msg already copy's length
        if (copy_to_user(buf + len, log->buffer, count - len))
            return -EFAULT;

    reader->r_off = logger_offset(log
            , reader->r_off + sizeof(struct logger_entry) + count);

    return get_user_hdr_len(reader->r_ver) + count;
}


static ssize_t logger_read(struct file* file
        , char __user * buf, size_t count, loff_t* pos) {
    struct logger_reader* reader = file->private_data;
    struct logger_log* log = reader->log;
    ssize_t ret;
    DEFINE_WAIT(wait);

    pr_info("into %s.\n", __FUNCTION__ );

start:
    while (1) {
        mutex_lock(&log->mutex);

        prepare_to_wait(&log->wq, &wait, TASK_INTERRUPTIBLE);

        ret = (log->w_off == reader->r_off);
        mutex_unlock(&log->mutex);
        if (!ret)
            break;

        if (file->f_flags & O_NONBLOCK) {
            ret = -EINTR;
            break;
        }

        if (signal_pending(current)) {
            ret = -EINTR;
            break;
        }

        schedule();
    }
    finish_wait(&log->wq, &wait);
    if (ret)
        return ret;

    mutex_lock(&log->mutex);

    if (!reader->r_all)
        reader->r_off = get_next_entry_by_uid(log
                , reader->r_off, current_euid());

    if (unlikely(log->w_off == reader->r_off)) {
        mutex_unlock(&log->mutex);
        goto start;
    }

    ret = get_user_hdr_len(reader->r_ver)
            + get_entry_msg_len(log, reader->r_off);
    // TODO("here we change from 'if(count < ret)' to ...")
    if (count < get_user_hdr_len(reader->r_ver)) {
        ret = -EINVAL;
        goto out;
    }

    ret = do_read_log_to_user(log, reader, buf, ret);
out:
    mutex_unlock(&log->mutex);

    return ret;
}


static inline struct logger_log* file_get_log(struct file* file) {
    if (file->f_mode & FMODE_READ) {
        struct logger_reader* reader = file->private_data;
        return reader->log;
    }
    return file->private_data;
}

static void fix_up_readers(struct logger_log* log, size_t len) {
    // TODO("when log_entry's header is split by two part")
}

/* write method, implementing support for write(),
 * writev(), and aio_write(). */
static ssize_t logger_write_iter(struct kiocb* iocb
        , struct iov_iter* from) {
    struct logger_log* log = file_get_log(iocb->ki_filp);
    struct logger_entry header;
    ktime_t now;
    size_t len, count, w_off;

    pr_info("into %s.\n", __FUNCTION__ );

    count = min_t(size_t, iov_iter_count(from), LOGGER_ENTRY_MAX_PAYLOAD);

    now = ktime_get();

    header.pid = current->tgid;
    header.tid = current->pid;
    header.nsec = (s32)do_div(now, 1000000000);
    header.sec = (s32)now;
    header.euid = current_euid();
    header.len = count;
    header.hdr_size = sizeof(struct logger_entry);

    if (unlikely(!header.len))
        return 0;

    mutex_lock(&log->mutex);

    fix_up_readers(log, sizeof(struct logger_entry) + header.len);

    // len reset with min (header, free space of buffer)
    len = min(sizeof(header), log->size - log->w_off);

    memcpy(log->buffer + log->w_off, &header, len);
    memcpy(log->buffer, (char*)&header + len, sizeof(header)-len);
    // until here, it just copies struct logger_entry's
    // variable header to buffer, size is sizeof(header)

    w_off = logger_offset(log, log->w_off + sizeof(struct logger_entry));

    len = min(count, log->size - w_off);

    if(copy_from_iter(log->buffer+w_off, len, from) != len) {
        mutex_unlock(&log->mutex);
        return -EFAULT;
    }
    if(copy_from_iter(log->buffer, count - len, from) != (count - len)) {
        mutex_unlock(&log->mutex);
        return -EFAULT;
    }
    // until here, it copies payload to addr after buffer+w_off

    log->w_off = logger_offset(log, w_off + count);



    char* str = vzalloc(count+1);
    memcpy(str, log->buffer + w_off, count);
    pr_info("log->w_off is %ld\n", log->w_off);
    pr_info("str is %s\n", str);
    vfree(str);



    mutex_unlock(&log->mutex);

    return len;
}




static struct logger_log* get_log_from_minor(int minor) {
    struct logger_log* log;
    list_for_each_entry(log, &log_list, logs) {
        if (log->misc.minor == minor)
            return log;
    }
    return NULL;
}

static int logger_open(struct inode* inode, struct file* file) {
    struct logger_log* log;
    int ret;

    pr_info("into %s.\n", __FUNCTION__ );

    ret = nonseekable_open(inode, file);
    if (ret)
        return ret;

    log = get_log_from_minor(MINOR(inode->i_rdev));
    if (!log)
        return -ENODEV;

    if (file->f_mode & FMODE_READ) {
        struct logger_reader* reader;

        reader = kmalloc(sizeof(*reader), GFP_KERNEL);
        if (!reader)
            return -ENODEV;

        reader->log = log;
        reader->r_ver = 1;
        reader->r_all = (in_egroup_p(inode->i_gid)
                || capable(CAP_SYSLOG));

        INIT_LIST_HEAD(&reader->list);

        mutex_lock(&log->mutex);
        reader->r_off = log->head;
        list_add_tail(&reader->list, &log->readers);
        mutex_unlock(&log->mutex);

        file->private_data = reader;
    } else {
        file->private_data = log;
    }

    return 0;
}

static int logger_release(struct inode* inode, struct file* file) {
    pr_info("into %s.\n", __FUNCTION__ );
    if (file->f_mode & FMODE_READ) {
        struct logger_reader* reader = file->private_data;
        struct logger_log* log = reader->log;

        mutex_lock(&log->mutex);
        list_del(&reader->list);
        mutex_unlock(&log->mutex);

        kfree(reader);
    }

    return 0;
}


static const struct file_operations logger_fops = {
        .owner      = THIS_MODULE,
        .read       = logger_read,
        .write_iter = logger_write_iter,/*
        .poll       = logger_poll,
        .unlocked_ioctl = logger_ioctl,*/
        .open       = logger_open,
        .release    = logger_release,
};

static int __init create_log(char* log_name, int size) {
    int ret = 0;
    struct logger_log* log;
    unsigned char* buffer;

    buffer = vmalloc(size);
    if (!buffer)
        return -ENOMEM;

    log = kzalloc(sizeof(*log), GFP_KERNEL);
    if (!log) {
        ret = -ENOMEM;
        goto out_free_buffer;
    }
    log->buffer = buffer;

    log->misc.minor = MISC_DYNAMIC_MINOR;
    log->misc.name = kstrdup(log_name, GFP_KERNEL);
    // log->misc.name = kstrndup(log_name, strlen(log_name), GFP_KERNEL);
    if (!log->misc.name) {
        ret = -ENOMEM;
        goto out_free_log;
    }

    log->misc.fops = &logger_fops;
    log->misc.parent = NULL;

    init_waitqueue_head(&log->wq);
    INIT_LIST_HEAD(&log->readers);
    mutex_init(&log->mutex);
    log->w_off = 0;
    log->head = 0;
    log->size = size;

    INIT_LIST_HEAD(&log->logs);
    list_add_tail(&log->logs, &log_list);

    ret = misc_register(&log->misc);
    if (unlikely(ret)) {
        pr_err("failed to register misc device for log");
        pr_err(": %s, ret = %d\n", log->misc.name, ret);
        goto out_free_misc_name;
    }

    pr_info("create %luK log '%s'\n"
           , (unsigned long)log->size >> 10, log->misc.name);
    return 0;

out_free_misc_name:
    kfree(log->misc.name);

out_free_log:
    kfree(log);

out_free_buffer:
    vfree(buffer);

    return ret;
}

static int __init logger_init(void) {
    int ret = 0;

    ret = create_log(LOGGER_LOG_MAIN, 512 * 1024);
    if (unlikely(ret))
        goto out;

    ret = create_log(LOGGER_LOG_SYSTEM, 1 * 1024);
    if (unlikely(ret))
        goto out;

    ret = create_log(LOGGER_LOG_EVENTS, 1 * 1024);
    if (unlikely(ret))
        goto out;

    ret = create_log(LOGGER_LOG_RADIO, 1 * 1024);
    if (unlikely(ret))
        goto out;

out:
    return ret;
}

static void __exit logger_exit(void) {
    struct logger_log* cur, *next;
    list_for_each_entry_safe(cur, next, &log_list, logs) {
        misc_deregister(&cur->misc);
        vfree(cur->buffer);
        kfree(cur->misc.name);
        list_del(&cur->logs);
        kfree(&cur->logs);
    }
}


module_init(logger_init);
module_exit(logger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xiexuyuyan, <2351783158@qq.com>");
MODULE_DESCRIPTION("droid Logger");