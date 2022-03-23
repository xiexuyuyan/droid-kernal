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

#ifndef SZ_1K
#define SZ_1K 0x400
#endif

#define FORBIDDEN_MMAP_FLAGS (VM_WRITE)

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
#ifdef WSL
                 , task_tgid_vnr(current)
                 , task_pid_vnr(current));
#else
                 , current->group_leader->pid
                 , current->pid);
#endif

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
    pid_t pid;

#ifdef WSL
    pid = task_pid_vnr(current);
#else
    pid = current->pid;
#endif

    while (*p) {
        parent = *p;
        thread = rb_entry(parent, struct binder_thread, rb_node);

        // todo(20. what mean to thread in rb_tree'left or right?)
        if (pid < thread->pid)
            p = &(*p)->rb_left;
        else if (pid > thread->pid)
            p = &(*p)->rb_right;
        else
            return thread;
    }

    if (!new_thread)
        return NULL;

    thread = new_thread;
    binder_stats_created(BINDER_STAT_THREAD);
    thread->proc = proc;
#ifdef WSL
    thread->pid = task_pid_vnr(current);
#else
    thread->pid = current->pid;
#endif
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
                 , proc->pid
#ifdef WSL
                 , task_pid_vnr(current)
#else
                 , current->pid
#endif
                 , cmd, arg);

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
        case BINDER_SET_MAX_THREADS: {
            int max_threads;
            if (copy_from_user(
                    &max_threads
                    , ubuf
                    , sizeof(max_threads))) {
                ret = -EINVAL;
                goto err;
            }
            binder_inner_proc_lock(proc);
            proc->max_threads = max_threads;
            binder_inner_proc_unlock(proc);
            break;
        }
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
                , proc->pid
#ifdef WSL
                , task_pid_vnr(current)
#else
                , current->pid
#endif
                , cmd, arg, ret);

err_unlocked:
    // todo(18. trace release-18 lock)
    return ret;
}

static void binder_vma_open(struct vm_area_struct* vma) {
    struct binder_proc* proc = vma->vm_private_data;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d %lx-%lx (%ld K) vma %lx page %lx\n"
                 , __FUNCTION__, proc->pid, vma->vm_start, vma->vm_end
                 , (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags
                 , (unsigned long)pgprot_val(vma->vm_page_prot));
    // todo(20220323-112324 why is there null implementation about open?)
}
static void binder_vma_close(struct vm_area_struct* vma) {
    struct binder_proc* proc = vma->vm_private_data;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d %lx-%lx (%ld K) vma %lx page %lx\n"
                 , __FUNCTION__, proc->pid, vma->vm_start, vma->vm_end
                 , (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags
                 , (unsigned long)pgprot_val(vma->vm_page_prot));
    binder_alloc_vma_close(&proc->alloc);
}

static vm_fault_t binder_vm_fault(struct vm_fault* vmf) {
    return VM_FAULT_SIGBUS;
}

static const struct vm_operations_struct binder_vm_ops = {
        .open = binder_vma_open,
        .close = binder_vma_close,
        .fault = binder_vm_fault,
};

static int binder_mmap(struct file* file, struct vm_area_struct* vma) {
    int ret;
    struct binder_proc* proc = file->private_data;
    const char* failure_string;

    if (proc->tsk != current->group_leader)
        return -EINVAL;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d %lx-%lx (%ld K) vma %lx page %lx\n"
                 , __FUNCTION__, proc->pid, vma->vm_start, vma->vm_end
                 , (vma->vm_end - vma->vm_start) / SZ_1K, vma->vm_flags
                 , (unsigned long)pgprot_val(vma->vm_page_prot));

    if (vma->vm_flags & FORBIDDEN_MMAP_FLAGS) {
        ret = -EPERM;
        failure_string = "bad vm_flags";
        goto err_bad_arg;
    }
    vma->vm_flags |= VM_DONTCOPY | VM_MIXEDMAP;
    vma->vm_flags &= ~VM_MAYWRITE;

    vma->vm_ops = &binder_vm_ops;
    vma->vm_private_data = proc;

    ret = binder_alloc_mmap_handler(&proc->alloc, vma);
    if (ret)
        return ret;
    return 0;

err_bad_arg:
    pr_err("%s: %d %lx-%lx %s failed %d\n"
           , __FUNCTION__
           , proc->pid
           , vma->vm_start, vma->vm_end
           , failure_string, ret);

    return ret;
}

const struct file_operations binder_fops = {
        .owner = THIS_MODULE,
        .open = binder_open,
        .unlocked_ioctl = binder_ioctl,
        .compat_ioctl = binder_ioctl,
        .mmap = binder_mmap,
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