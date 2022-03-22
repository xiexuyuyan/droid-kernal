#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/freezer.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/nsproxy.h>
#include <linux/poll.h>
#include <linux/debugfs.h>
#include <linux/rbtree.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/pid_namespace.h>
#include <linux/security.h>
#include <linux/spinlock.h>
#include <linux/ratelimit.h>
#include <linux/syscalls.h>
#include <linux/task_work.h>
//#include <linux/android_vendor.h>

#include <uapi/linux/sched/types.h>
#include <uapi/linux/android/binder.h>

#include <asm/cacheflush.h>

#include "binder_internal.h"
#include "binder_fs.h"

static HLIST_HEAD(binder_procs);
static DEFINE_MUTEX(binder_procs_lock);

enum {
    BINDER_DEBUG_USER_ERROR         = 1U << 0,
    BINDER_DEBUG_FAILED_TRANSACTION = 1U << 1,
    BINDER_DEBUG_DEAD_TRANSACTION   = 1U << 2,
    BINDER_DEBUG_OPEN_CLOSE         = 1U << 3,
    BINDER_DEBUG_SPINLOCKS          = 1U << 15,
};

static uint32_t binder_debug_mask =
        BINDER_DEBUG_USER_ERROR
        | BINDER_DEBUG_FAILED_TRANSACTION
        | BINDER_DEBUG_DEAD_TRANSACTION
        | BINDER_DEBUG_OPEN_CLOSE;
module_param_named(debug_mask, binder_debug_mask, uint, 0644);

static DECLARE_WAIT_QUEUE_HEAD(binder_user_error_wait);
static int binder_stop_on_user_error;

#define debug_binder(mask, x...) \
    do { \
        if(binder_debug_mask & mask) \
            pr_info_ratelimited(x); \
    } while(0)

char* binder_devices_param = "binder,hwbinder,vndbinder";

struct binder_transaction_log binder_transaction_log;
struct binder_transaction_log binder_transaction_log_failed;

static struct binder_stats binder_stats;

static inline void binder_stats_created(enum binder_stat_types type) {
    atomic_inc(&binder_stats.obj_created[type]);
}

static inline void binder_stats_deleted(enum binder_stat_types type) {
    atomic_inc(&binder_stats.obj_deleted[type]);
}

#define binder_inner_proc_lock(_proc) _binder_inner_proc_lock(_proc, __LINE__)
static void _binder_inner_proc_lock(
        struct binder_proc* proc, int line)
                __acquires(&proc->inner_lock) {
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n"
                 , __FUNCTION__, line);
    spin_lock(&proc->inner_lock);
}

#define binder_inner_proc_unlock(_proc) _binder_inner_proc_unlock(_proc, __LINE__)
static void _binder_inner_proc_unlock(
        struct binder_proc* proc, int line)
                __releases(&proc->inner_lock) {
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n"
                 , __FUNCTION__, line);
    spin_unlock(&proc->inner_lock);
}



static bool is_rt_policy(int policy) {
    return policy == SCHED_FIFO || policy == SCHED_RR;
}

static bool is_fair_policy(int policy) {
    return policy == SCHED_NORMAL || policy == SCHED_BATCH;
}

static bool binder_supported_policy(int policy) {
    return is_rt_policy(policy) || is_fair_policy(policy);
}

static int binder_open(struct inode* inode, struct file* file) {
    struct binder_proc* proc, * itr;
    struct binder_device* binder_dev;
    struct binderfs_info* info;
    struct dentry* binder_binderfs_dir_entry_proc = NULL;
    bool existing_pid = false;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d:%d\n"
                 , __FUNCTION__
                 , current->group_leader->pid
                 , current->pid);

    proc = kzalloc(sizeof(*proc), GFP_KERNEL);
    if (proc == NULL)
        return -ENOMEM;

    spin_lock_init(&proc->inner_lock);
    spin_lock_init(&proc->outer_lock);
    get_task_struct(current->group_leader);
    proc->tsk = current->group_leader;
    INIT_LIST_HEAD(&proc->todo);
    init_waitqueue_head(&proc->freeze_wait);
    if (binder_supported_policy(current->policy)) {
        proc->default_priority.sched_policy = current->policy;
        proc->default_priority.prio = current->normal_prio;
    } else {
        proc->default_priority.sched_policy = SCHED_NORMAL;
        proc->default_priority.prio = NICE_TO_PRIO(0);
    }

    if (is_binderfs_device(inode)) {
        binder_dev = inode->i_private;
        info = inode->i_sb->s_fs_info;
        binder_binderfs_dir_entry_proc = info->proc_log_dir;
    } else {
        binder_dev = container_of(file->private_data
                , struct binder_device, miscdev);
    }
    refcount_inc(&binder_dev->ref);
    proc->context = &binder_dev->context;
    binder_alloc_init(&proc->alloc);

    binder_stats_created(BINDER_STAT_PROC);
    proc->pid = current->group_leader->pid;
    INIT_LIST_HEAD(&proc->delivered_death);
    INIT_LIST_HEAD(&proc->waiting_threads);
    file->private_data = proc;

    mutex_lock(&binder_procs_lock);
    hlist_for_each_entry(itr, &binder_procs, proc_node) {
        if (itr->pid == proc->pid) {
            existing_pid = true;
            break;
        }
    }
    hlist_add_head(&proc->proc_node, &binder_procs);
    mutex_unlock(&binder_procs_lock);

    // todo(17. debugfs)

    return 0;
}

static struct binder_thread* binder_get_thread_ilocked(
        struct binder_proc* proc, struct binder_thread* new_thread) {
    struct binder_thread* thread = NULL;
    struct rb_node* parent = NULL;
    struct rb_node** p = &proc->threads.rb_node;

    while (*p) {
        parent = *p;
        thread = rb_entry(parent, struct binder_thread, rb_node);

        // todo(20. what mean to thread in rb_tree'left or right?)
        if (current->pid < thread->pid)
            p = &(*p)->rb_left;
        else if (current->pid > thread->pid)
            p = &(*p)->rb_right;
        else
            return thread;
    }

    if (!new_thread)
        return NULL;

    thread = new_thread;
    binder_stats_created(BINDER_STAT_THREAD);
    thread->proc = proc;
    thread->pid = current->pid;
    get_task_struct(current);
    thread->task = current;
    atomic_set(&thread->tmp_ref, 0);
    init_waitqueue_head(&thread->wait);
    INIT_LIST_HEAD(&thread->todo);
    rb_link_node(&thread->rb_node, parent, p);
    rb_insert_color(&thread->rb_node, &proc->threads);
    thread->looper_need_return = true;
    thread->return_error.work.type = BINDER_WORK_RETURN_ERROR;
    thread->return_error.cmd = BR_OK;
    thread->reply_error.work.type = BINDER_WORK_RETURN_ERROR;
    thread->reply_error.cmd = BR_OK;
    INIT_LIST_HEAD(&new_thread->waiting_thread_node);
    return thread;
}

static struct binder_thread* binder_get_thread(struct binder_proc*  proc) {
    struct binder_thread* thread;
    struct binder_thread* new_thread;

    binder_inner_proc_lock(proc);
    thread = binder_get_thread_ilocked(proc, NULL);
    binder_inner_proc_unlock(proc);

    if (!thread) {
        new_thread = kzalloc(sizeof(*thread), GFP_KERNEL);
        if (new_thread == NULL)
            return NULL;
        binder_inner_proc_lock(proc);
        thread = binder_get_thread_ilocked(proc, new_thread);
        binder_inner_proc_unlock(proc);
        if (thread != new_thread)
            kfree(new_thread);
    }

    return thread;
}

static long binder_ioctl(
        struct file* file, unsigned int cmd, unsigned long arg) {
    int ret;
    struct  binder_proc* proc = file->private_data;
    struct binder_thread* thread;
    unsigned int size = _IOC_SIZE(cmd);
    void* __user ubuf = (void* __user)arg;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d:%d, cmd = %x, arg = %lx\n"
                 , __FUNCTION__
                 , proc->pid, current->pid, cmd, arg);

    // todo(18. trace)
    // todo(19. alloc self test)

    ret = wait_event_interruptible(binder_user_error_wait, binder_stop_on_user_error < 2);
    if (ret)
        goto err_unlocked;

    thread = binder_get_thread(proc);
    if (thread == NULL) {
        ret = -ENOMEM;
        goto err;
    }

    switch (cmd) {
        case BINDER_VERSION: {
            struct binder_version* __user ver = ubuf;
            if (size != sizeof(struct binder_version)) {
                ret = -EINVAL;
                goto err;
            }
            if (put_user(BINDER_CURRENT_PROTOCOL_VERSION
                         , &ver->protocol_version)) {
                ret = -EINVAL;
                goto err;
            }
            break;
        }
        default:
            ret = -EINVAL;
            goto err;
    }
    ret = 0;

err:
    if (thread)
        thread->looper_need_return = false;
    wait_event_interruptible(binder_user_error_wait, binder_stop_on_user_error < 2);
    if (ret && ret != -EINTR)
        pr_info("%s %d:%d, cmd = %x, arg = %lx, returned %d\n"
                , __FUNCTION__
                , proc->pid, current->pid, cmd, arg, ret);

err_unlocked:
    // todo(18. trace release-18 lock)
    return ret;
}

const struct file_operations binder_fops = {
        .owner = THIS_MODULE,
        .open = binder_open,
        .unlocked_ioctl = binder_ioctl,
        .compat_ioctl = binder_ioctl,
};


static int __init binder_init(void) {
    int ret;
/*
    char* device_name,* device_tmp;
    struct binder_device* device;
    // todo(1. struct binder_device)
    struct hlist_head* tmp;
    char* device_names = NULL;
*/

    ret = binder_alloc_shrinker_init();
    if (ret) return ret;

    atomic_set(&binder_transaction_log.cur, ~0U);
    atomic_set(&binder_transaction_log_failed.cur, ~0U);

    ret = init_binderfs();
    if (ret)
        goto err_init_binder_device_failed;

err_init_binder_device_failed:

    return ret;
}

static void __exit binder_exit(void) {
    pr_info("into %s.\n", __FUNCTION__ );
    binder_alloc_exit();
    binder_fs_exit();
}

module_init(binder_init);
module_exit(binder_exit);


MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("xiexuyuyan, <2351783158@qq.com>");
MODULE_DESCRIPTION("droid Binder");