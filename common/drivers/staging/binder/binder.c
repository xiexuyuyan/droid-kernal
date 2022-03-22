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
};

static uint32_t binder_debug_mask =
        BINDER_DEBUG_USER_ERROR
        | BINDER_DEBUG_FAILED_TRANSACTION
        | BINDER_DEBUG_DEAD_TRANSACTION
        | BINDER_DEBUG_OPEN_CLOSE;
module_param_named(debug_mask, binder_debug_mask, uint, 0644);

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


const struct file_operations binder_fops = {
        .owner = THIS_MODULE,
        .open = binder_open,
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