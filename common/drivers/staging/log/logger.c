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
#define unlikely(x) ((x) == 0)
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

static struct logger_log* get_log_from_minor(int minor) {
    struct logger_log* log;
    list_for_each_entry(log, &log_list, logs) {
        if (log->misc.minor == minor)
            return log;
    }
    return NULL;
}

static int logger_open(struct inode* inode, struct file* file) {
    pr_info("into %s.\n", __FUNCTION__ );
    struct logger_log* log;
    int ret;

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
        .owner      = THIS_MODULE,/*
        .read       = logger_read,
        .write_iter = logger_write_iter,
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
    // TODO("implicit declaration of kstrdup in c99 ")
    // log->misc.name = kstrdup(log_name, GFP_KERNEL);
    log->misc.name = kstrndup(log_name, strlen(log_name), GFP_KERNEL);
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
        // TODO printf("failed to register misc device for log");
        pr_err("failed to register misc device for log");
        // TODO printf(": %s, ret = %d\n", log->misc.name, ret);
        pr_err(": %s, ret = %d\n", log->misc.name, ret);
        goto out_free_misc_name;
    }

    // TODO printf("create %luK log '%s'\n"
    //        , (unsigned long)log->size >> 10, log->misc.name);
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