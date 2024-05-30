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

static DEFINE_SPINLOCK(binder_dead_nodes_lock);

static atomic_t binder_last_id;

#ifndef SZ_1K
#define SZ_1K 0x400
#endif

#define FORBIDDEN_MMAP_FLAGS (VM_WRITE)

// todo(20220428-095431
//  common/include/uapi/linux/android/binder.h +=82)
enum flat_binder_object_shifts {
    FLAT_BINDER_FLAG_SCHED_POLICY_SHIFT = 9,
};
enum {
    FLAT_BINDER_FLAG_SCHED_POLICY_MASK =
            3U << FLAT_BINDER_FLAG_SCHED_POLICY_SHIFT
};

enum {
    BINDER_DEBUG_USER_ERROR         = 1U << 0,
    BINDER_DEBUG_FAILED_TRANSACTION = 1U << 1,
    BINDER_DEBUG_DEAD_TRANSACTION   = 1U << 2,
    BINDER_DEBUG_OPEN_CLOSE         = 1U << 3,
    BINDER_DEBUG_WRITE_READ         = 1U << 6,
    BINDER_DEBUG_TRANSACTION        = 1U << 9,
    BINDER_DEBUG_INTERNAL_REFS      = 1U << 12,
    BINDER_DEBUG_SPINLOCKS          = 1U << 15,
};

enum {
    BINDER_LOOPER_STATE_REGISTERED  = 0x01,
    BINDER_LOOPER_STATE_ENTERED     = 0x02,
    BINDER_LOOPER_STATE_EXITED      = 0x04,
    BINDER_LOOPER_STATE_INVALID     = 0x08,
    BINDER_LOOPER_STATE_WAITING     = 0x08,
    BINDER_LOOPER_STATE_POLL        = 0x20,
};

static uint32_t binder_debug_mask =
        BINDER_DEBUG_USER_ERROR
        | BINDER_DEBUG_FAILED_TRANSACTION
        | BINDER_DEBUG_DEAD_TRANSACTION
        | BINDER_DEBUG_OPEN_CLOSE
        | BINDER_DEBUG_WRITE_READ
        | BINDER_DEBUG_TRANSACTION
        | BINDER_DEBUG_INTERNAL_REFS
        | BINDER_DEBUG_SPINLOCKS;
module_param_named(debug_mask, binder_debug_mask, uint, 0644);

static DECLARE_WAIT_QUEUE_HEAD(binder_user_error_wait);
static int binder_stop_on_user_error;

#define debug_binder(mask, x...) \
    do { \
        if(binder_debug_mask & mask) \
            pr_info_ratelimited(x); \
    } while(0)

char* binder_devices_param = "binder,hwbinder,vndbinder";

static struct binder_stats binder_stats;

static inline void binder_stats_created(enum binder_stat_types type) {
    atomic_inc(&binder_stats.obj_created[type]);
}

static inline void binder_stats_deleted(enum binder_stat_types type) {
    atomic_inc(&binder_stats.obj_deleted[type]);
}

struct binder_transaction_log binder_transaction_log;
struct binder_transaction_log binder_transaction_log_failed;

static struct binder_transaction_log_entry* binder_transaction_log_add(
        struct binder_transaction_log* log) {
    struct binder_transaction_log_entry* e;
    unsigned int cur = atomic_inc_return(&log->cur);

    if (cur >= ARRAY_SIZE(log->entry)) {
        log->full = true;
    }
    e = &log->entry[cur % ARRAY_SIZE(log->entry)];
    WRITE_ONCE(e->debug_id_done, 0);
    // todo(20220419-192542 memory write-barrier)
    smp_wmb();
    memset(e, 0, sizeof(*e));
    return e;
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

#define binder_node_lock(node) _binder_node_lock(node, __LINE__)
static void _binder_node_lock(struct binder_node* node, int line)
        __acquires(&node->lock){
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n", __FUNCTION__, line);
    spin_lock(&node->lock);
}

#define binder_node_unlock(node) _binder_node_unlock(node, __LINE__)
static void _binder_node_unlock(struct binder_node* node, int line)
        __releases(&node->lock){
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n", __FUNCTION__, line);
    spin_unlock(&node->lock);
}

#define binder_node_inner_lock(node) _binder_node_inner_lock(node, __LINE__)
static void _binder_node_inner_lock(struct binder_node* node, int line)
        __acquires(&node->lock) __acquires(&node->proc->inner_lock) {
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n", __FUNCTION__, line);
    spin_lock(&node->lock);
    if (node->proc) {
        binder_inner_proc_lock(node->proc);
    } else {
        // todo(20220428-165121 why? if node->proc is nullptr...)
        __acquire(&node->proc->inner_lock);
    }
}

#define binder_node_inner_unlock(node) _binder_node_inner_unlock(node, __LINE__)
static void _binder_node_inner_unlock(struct binder_node* node, int line)
        __releases(&node->lock) __releases(&node->proc->inner_lock) {
    debug_binder(BINDER_DEBUG_SPINLOCKS
                 , "%s: line=%d\n", __FUNCTION__, line);
    if (node->proc) {
        binder_inner_proc_unlock(node->proc);
    } else {
        __release(&node->proc->inner_lock);
    }
    spin_unlock(&node->lock);
}

static bool binder_worklist_empty_ilocked(struct list_head* list) {
    return list_empty(list);
}

static void binder_enqueue_work_ilocked(
        struct binder_work* work
        , struct list_head* target_list) {
    BUG_ON(target_list == NULL);
    BUG_ON(work->entry.next && !list_empty(&work->entry));
    list_add_tail(&work->entry, target_list);
}

static void binder_enqueue_deferred_thread_work_ilocked(
        struct binder_thread* thread
        , struct binder_work* work) {
    WARN_ON(!list_empty(&thread->waiting_thread_node));
    binder_enqueue_work_ilocked(work, &thread->todo);
}

static void binder_dequeue_work_ilocked(struct binder_work* work) {
    list_del_init(&work->entry);
}

static bool binder_available_for_proc_work_ilocked(
        struct binder_thread* thread) {
    return !thread->transaction_stack
            && binder_worklist_empty_ilocked(&thread->todo)
            && (thread->looper & (BINDER_LOOPER_STATE_ENTERED
                | BINDER_LOOPER_STATE_REGISTERED));
}

static void binder_wakeup_poll_thread_ilocked(
        struct binder_proc* proc, bool sync) {
    struct rb_node* n;
    struct binder_thread* thread;
    for (n = rb_first(&proc->threads); n != NULL; n = rb_next(n)) {
        thread = rb_entry(n, struct binder_thread, rb_node);
        if (thread->looper & BINDER_LOOPER_STATE_POLL
            && binder_available_for_proc_work_ilocked(thread)) {
            if (sync) {
                wake_up_interruptible_sync(&thread->wait);
            } else {
                wake_up_interruptible(&thread->wait);
            }
        }
    }
}

static struct binder_thread* binder_select_thread_ilocked(struct binder_proc* proc) {
    struct binder_thread* thread;
    assert_spin_locked(&proc->inner_lock);
    thread = list_first_entry_or_null(&proc->waiting_threads
            , struct binder_thread
            , waiting_thread_node);
    if (thread) {
        list_del_init(&thread->waiting_thread_node);
    }

    return thread;
}

static void binder_wakeup_thread_ilocked(
        struct binder_proc* proc
        , struct binder_thread* thread
        , bool sync) {
    assert_spin_locked(&proc->inner_lock);
    if (thread) {
        if (sync) {
            wake_up_interruptible_sync(&thread->wait);
        } else {
            wake_up_interruptible(&thread->wait);
        }
        return;
    }
    binder_wakeup_poll_thread_ilocked(proc, sync);
}

static void binder_wakeup_proc_ilocked(struct binder_proc* proc) {
    struct binder_thread* thread = binder_select_thread_ilocked(proc);
    binder_wakeup_thread_ilocked(proc, thread, false);
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

static int to_kernel_prio(int policy, int user_priority) {
    if (is_fair_policy(policy)) {
        return NICE_TO_PRIO(user_priority);
    } else {
        return MAX_RT_PRIO - 1 - user_priority;
    }
}

static int binder_inc_node_nilocked(
        struct binder_node* node, int strong, int internal
        , struct list_head* target_list) {
    struct binder_proc* proc = node->proc;
    assert_spin_locked(&node->lock);
    if (proc) {
        assert_spin_locked(&proc->inner_lock);
    }
    if (strong) {
        if (internal) {
            if (target_list == NULL
                && node->internal_strong_refs == 0
                && !(node->proc
                    && node ==
                        node->proc->context->binder_context_mgr_node
                    && node->has_strong_ref)) {
                pr_err("invalid inc strong node for %d\n"
                       , node->debug_id);
            }
            node->internal_strong_refs++;
        } else {
            node->local_strong_refs++;
        }
        if (!node->has_strong_ref && target_list) {
            struct binder_thread* thread = container_of(
                    target_list, struct binder_thread, todo);
            binder_dequeue_work_ilocked(&node->work);
            BUG_ON(&thread->todo != target_list);
            binder_enqueue_deferred_thread_work_ilocked(
                    thread, &node->work);
        }
    } else {
        if (!internal) {
            node->local_weak_refs++;
        }
        if (!node->has_weak_ref && list_empty(&node->work.entry)) {
            if (target_list == NULL) {
                pr_err("invalid inc weak node for %d\n", node->debug_id);
                return -EINVAL;
            }
            binder_enqueue_work_ilocked(&node->work, target_list);
        }
    }

    return 0;
}

static bool binder_dec_node_nilocked(
        struct binder_node* node, int strong, int internal) {
    struct binder_proc* proc = node->proc;

    assert_spin_locked(&node->lock);
    if (proc) {
        assert_spin_locked(&proc->inner_lock);
    }
    if (strong) {
        if (internal) {
            node->internal_strong_refs--;
        } else {
            node->local_strong_refs--;
        }
        if (node->local_strong_refs || node->internal_strong_refs) {
            return false;
        }
    } else {
        if (!internal) {
            node->local_weak_refs--;
        }
        if (node->local_weak_refs
                || node->tmp_refs
                || !hlist_empty(&node->refs)) {
            return false;
        }
    }

    if (proc && (node->has_strong_ref || node->has_weak_ref)) {
        if (list_empty(&node->work.entry)) {
            // todo(20220428-182704
            //  it means join some work into proc auto?)
            binder_enqueue_work_ilocked(&node->work, &proc->todo);
            binder_wakeup_proc_ilocked(proc);
        }
    } else {
        if (!hlist_empty(&node->refs)
                && !node->local_strong_refs
                && !node->local_weak_refs
                && !node->tmp_refs) {
            if (proc) {
                binder_dequeue_work_ilocked(&node->work);
                rb_erase(&node->rb_node, &proc->nodes);
            } else {
                BUG_ON(!list_empty(&node->work.entry));
                spin_lock(&binder_dead_nodes_lock);
                // tmp_refs could have changed ,so check it again
                if (node->tmp_refs) {
                    spin_unlock(&binder_dead_nodes_lock);
                    return false;
                }
                // todo(20220428-182823 how to justify it is dead)
                hlist_del(&node->dead_node);
                spin_unlock(&binder_dead_nodes_lock);
                debug_binder(BINDER_DEBUG_INTERNAL_REFS
                             , "dead node %d deleted\n"
                             , node->debug_id);
            }
            return true;
        }
    }
    return false;
}

static void binder_inc_node_tmpref_ilocked(struct binder_node* node) {
    node->tmp_refs++;
}

static void binder_free_node(struct binder_node* node) {
    kfree(node);
    binder_stats_deleted(BINDER_STAT_NODE);
}

static struct binder_node* binder_init_node_ilocked(
        struct binder_proc* proc
        , struct binder_node* new_node
        , struct flat_binder_object* fp) {
    struct rb_node** p = &proc->nodes.rb_node;
    struct rb_node* parent = NULL;
    struct binder_node* node;
    binder_uintptr_t ptr = fp ? fp->binder : 0;
    binder_uintptr_t cookie = fp ? fp->cookie : 0;
    __u32 flags = fp ? fp->flags : 0;
    s8 priority;

    assert_spin_locked(&proc->inner_lock);

    while (*p) {
        parent = *p;
        node = rb_entry(parent, struct binder_node, rb_node);

        if (ptr < node->ptr) {
            p = &(*p)->rb_left;
        } else if (ptr > node->ptr) {
            p = &(*p)->rb_right;
        } else {
            binder_inc_node_tmpref_ilocked(node);
            return node;
        }
    }
    node = new_node;
    binder_stats_created(BINDER_STAT_NODE);
    node->tmp_refs++;
    rb_link_node(&node->rb_node, parent, p);
    // todo(20220428-093657 how doe struct rb_node manage a rb tree?)
    rb_insert_color(&node->rb_node, &proc->nodes);
    node->debug_id = atomic_inc_return(&binder_last_id);
    node->proc = proc;
    node->ptr = ptr;
    node->cookie = cookie;
    node->work.type = BINDER_WORK_NODE;
    priority = flags & FLAT_BINDER_FLAG_PRIORITY_MASK;
    node->sched_policy = (flags & FLAT_BINDER_FLAG_SCHED_POLICY_MASK)
            >> FLAT_BINDER_FLAG_SCHED_POLICY_SHIFT;
    node->min_priority = to_kernel_prio(node->sched_policy, priority);
    node->accept_fds = !!(flags & FLAT_BINDER_FLAG_ACCEPTS_FDS);
    node->inherit_rt = !!(flags & FLAT_BINDER_FLAG_TXN_SECURITY_CTX);
    spin_lock_init(&node->lock);
    INIT_LIST_HEAD(&node->work.entry);
    INIT_LIST_HEAD(&node->async_todo);
    debug_binder(BINDER_DEBUG_INTERNAL_REFS,
                 "%d:%d node %d u%016llx c%016llx created\n"
                 , proc->pid, current->pid, node->debug_id
                 , (u64)node->ptr, (u64)node->cookie);
    return node;
}

static void binder_dec_node_tmpref(struct binder_node* node) {
    bool free_node;

    binder_node_inner_lock(node);
    if (!node->proc) {
        spin_lock(&binder_dead_nodes_lock);
    } else {
        __acquire(&binder_dead_nodes_lock);
    }
    node->tmp_refs--;
    BUG_ON(node->tmp_refs < 0);
    if (!node->proc) {
        spin_unlock(&binder_dead_nodes_lock);
    } else {
        __release(&binder_dead_nodes_lock);
    }
    free_node = binder_dec_node_nilocked(node, 0 , 1);
    binder_node_inner_unlock(node);
    if (free_node)
        binder_free_node(node);
}

static void binder_put_node(struct binder_node* node) {
    binder_dec_node_tmpref(node);
}
static struct binder_node* binder_new_node(
        struct binder_proc* proc
        , struct flat_binder_object* fp) {
    struct binder_node* node;
    struct binder_node* new_node = kzalloc(sizeof(*node), GFP_KERNEL);

    if (!new_node) {
        return NULL;
    }
    binder_inner_proc_lock(proc);
    node = binder_init_node_ilocked(proc, new_node, fp);
    binder_inner_proc_unlock(proc);
    if (node != new_node) {
        // The node was already added by another thread
        kfree(new_node);
    }

    return node;
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
#ifdef __WSL__
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
#ifdef __WSL__
    proc->pid = task_tgid_vnr(current);
#else
    proc->pid = current->group_leader->pid;
#endif
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

static struct binder_node* binder_get_node_refs_for_txn(
        struct binder_node* node
        , struct binder_proc** p_proc
        , uint32_t* error) {
    struct binder_node* target_node = NULL;
    binder_node_inner_lock(node);
    if (node->proc) {
        target_node = node;
        // node->local_strong_refs++;
        binder_inc_node_nilocked(node, 1, 0, NULL);
        // node->tmp_refs++;
        binder_inc_node_tmpref_ilocked(node);
        node->proc->tmp_ref++;
        *p_proc = node->proc;
    } else {
        *error = BR_DEAD_REPLY;
    }
    binder_node_inner_unlock(node);

    return target_node;
}

static void binder_transaction(struct binder_proc *proc,
                               struct binder_thread *thread,
                               struct binder_transaction_data *tr, int reply,
                               binder_size_t extra_buffers_size)
{
    int ret;
    struct binder_transaction *t;
    struct binder_work *w;
    struct binder_work *tcomplete;
    binder_size_t buffer_offset = 0;
    binder_size_t off_start_offset, off_end_offset;
    binder_size_t off_min;
    binder_size_t sg_buf_offset, sg_buf_end_offset;
    binder_size_t user_offset = 0;
    struct binder_proc *target_proc = NULL;
    struct binder_thread *target_thread = NULL;
    struct binder_node *target_node = NULL;
    struct binder_transaction *in_reply_to = NULL;
    struct binder_transaction_log_entry *e;
    uint32_t return_error = 0;
    uint32_t return_error_param = 0;
    uint32_t return_error_line = 0;
    binder_size_t last_fixup_obj_off = 0;
    binder_size_t last_fixup_min_off = 0;
    struct binder_context *context = proc->context;
    int t_debug_id = atomic_inc_return(&binder_last_id);
    char *secctx = NULL;
    u32 secctx_sz = 0;
    const void __user *user_buffer = (const void __user *)
            (uintptr_t)tr->data.ptr.buffer;
    bool is_nested = false;

    debug_binder(BINDER_DEBUG_TRANSACTION
        , "%s: start.\n"
        , __FUNCTION__);

    e = binder_transaction_log_add(&binder_transaction_log);
    e->debug_id = t_debug_id;
    e->call_type = reply ? 2 : !!(tr->flags & TF_ONE_WAY);
    e->from_proc = proc->pid;
    e->from_thread = thread->pid;
    e->target_handle = tr->target.handle;
    e->data_size = tr->data_size;
    e->offsets_size = tr->offsets_size;
    strscpy(e->context_name, proc->context->name, BINDERFS_MAX_NAME);

    if (reply) {} else {
        if (tr->target.handle) {} else {
            mutex_lock(&context->context_mgr_node_lock);
            target_node = context->binder_context_mgr_node;
            if (target_node)
                target_node = binder_get_node_refs_for_txn(
                        target_node, &target_proc,
                        &return_error);
            else
                return_error = BR_DEAD_REPLY;
            mutex_unlock(&context->context_mgr_node_lock);
            if (target_node && target_proc->pid == proc->pid) {}
        }
        if (!target_node) {}
        e->to_node = target_node->debug_id;
        if (WARN_ON(proc == target_proc)) {}
        binder_inner_proc_lock(proc);

        w = list_first_entry_or_null(&thread->todo,
                                     struct binder_work, entry);
        if (!(tr->flags & TF_ONE_WAY) && w &&
            w->type == BINDER_WORK_TRANSACTION) {}

        if (!(tr->flags & TF_ONE_WAY) && thread->transaction_stack) {}
        binder_inner_proc_unlock(proc);
    }
    if (target_thread)
        e->to_thread = target_thread->pid;
    e->to_proc = target_proc->pid;

    /* TODO: reuse incoming transaction for reply */
    t = kzalloc(sizeof(*t), GFP_KERNEL);
    if (t == NULL) {}
    INIT_LIST_HEAD(&t->fd_fixups);
    binder_stats_created(BINDER_STAT_TRANSACTION);
    spin_lock_init(&t->lock);

    tcomplete = kzalloc(sizeof(*tcomplete), GFP_KERNEL);
    if (tcomplete == NULL) {}
    binder_stats_created(BINDER_STAT_TRANSACTION_COMPLETE);

    t->debug_id = t_debug_id;

    if (!reply && !(tr->flags & TF_ONE_WAY))
        t->from = thread;
    else
        t->from = NULL;
    t->sender_euid = task_euid(proc->tsk);
    t->to_proc = target_proc;
    t->to_thread = target_thread;
    t->code = tr->code;
    t->flags = tr->flags;
    t->is_nested = is_nested;
    if (!(t->flags & TF_ONE_WAY) &&
        binder_supported_policy(current->policy)) {
        /* Inherit supported policies for synchronous transactions */
        t->priority.sched_policy = current->policy;
        t->priority.prio = current->normal_prio;
    } else {
        /* Otherwise, fall back to the default priority */
        t->priority = target_proc->default_priority;
    }
/*
    t->buffer = binder_alloc_new_buf(&target_proc->alloc, tr->data_size,
                                     tr->offsets_size, extra_buffers_size,
                                     !reply && (t->flags & TF_ONE_WAY), current->tgid);
    if (IS_ERR(t->buffer)) {}
    if (secctx) {}
    t->buffer->debug_id = t->debug_id;
    t->buffer->transaction = t;
    t->buffer->target_node = target_node;
    t->buffer->clear_on_free = !!(t->flags & TF_CLEAR_BUF);

    if (binder_alloc_copy_user_to_buffer(
            &target_proc->alloc,
            t->buffer,
            ALIGN(tr->data_size, sizeof(void *)),
            (const void __user *)
                    (uintptr_t)tr->data.ptr.offsets,
            tr->offsets_size)) {}
    if (!IS_ALIGNED(tr->offsets_size, sizeof(binder_size_t))) {}
    if (!IS_ALIGNED(extra_buffers_size, sizeof(u64))) {}
    off_start_offset = ALIGN(tr->data_size, sizeof(void *));
    buffer_offset = off_start_offset;
    off_end_offset = off_start_offset + tr->offsets_size;
    sg_buf_offset = ALIGN(off_end_offset, sizeof(void *));
    sg_buf_end_offset = sg_buf_offset + extra_buffers_size -
                        ALIGN(secctx_sz, sizeof(u64));
    off_min = 0;
    for (buffer_offset = off_start_offset; buffer_offset < off_end_offset;
         buffer_offset += sizeof(binder_size_t)) {}
    if (binder_alloc_copy_user_to_buffer(
            &target_proc->alloc,
            t->buffer, user_offset,
            user_buffer + user_offset,
            tr->data_size - user_offset)) {}
    tcomplete->type = BINDER_WORK_TRANSACTION_COMPLETE;
    t->work.type = BINDER_WORK_TRANSACTION;

    if (reply) {} else if (!(t->flags & TF_ONE_WAY)) {
        BUG_ON(t->buffer->async_transaction != 0);
        binder_inner_proc_lock(proc);
        binder_enqueue_deferred_thread_work_ilocked(thread, tcomplete);
        t->need_reply = 1;
        t->from_parent = thread->transaction_stack;
        thread->transaction_stack = t;
        binder_inner_proc_unlock(proc);
        return_error = binder_proc_transaction(t,
                                               target_proc, target_thread);
        if (return_error) {}
    } else {}
    if (target_thread)
        binder_thread_dec_tmpref(target_thread);
    binder_proc_dec_tmpref(target_proc);
*/
    if (target_node)
        binder_dec_node_tmpref(target_node);
    /*
     * write barrier to synchronize with initialization
     * of log entry
     */
    smp_wmb();
    WRITE_ONCE(e->debug_id_done, t_debug_id);
    debug_binder(BINDER_DEBUG_TRANSACTION
            , "%s: end\n"
            , __FUNCTION__);
    return;
}

static int binder_thread_write(
        struct binder_proc* proc
        , struct binder_thread* thread
        , binder_uintptr_t binder_buffer
        , size_t size
        , binder_size_t* consumed) {
    uint32_t cmd;
    struct binder_context* context = proc->context;
    void __user* buffer = (void __user*)(uintptr_t)binder_buffer;
    void __user* ptr = buffer + *consumed;
    void __user* end = buffer + size;

    while (ptr < end && thread->return_error.cmd == BR_OK) {
        int ret;

        if (get_user(cmd, (uint32_t __user*)ptr)) {
            return -EFAULT;
        }
        ptr += sizeof(uint32_t);
        // todo(20220419-151139 trace)
        if (_IOC_NR(cmd) < ARRAY_SIZE(binder_stats.bc)) {
            // todo(20220419-153415 I know it's to set binder_stats count
            //  , but when does it uses it?)
            atomic_inc(&binder_stats.bc[_IOC_NR(cmd)]);
            atomic_inc(&proc->stats.bc[_IOC_NR(cmd)]);
            atomic_inc(&thread->stats.bc[_IOC_NR(cmd)]);
        }

        debug_binder(BINDER_DEBUG_WRITE_READ
                     , "%s: %d:%d cmd(%x)\n"
                     , __FUNCTION__, proc->pid
#ifdef __WSL__
                     , task_pid_vnr(current)
#else
                     , current->pid
#endif
                     , cmd);

        switch (cmd) {
            case BC_TRANSACTION:
            case BC_REPLY: {
                struct binder_transaction_data tr;
                if (copy_from_user(&tr, ptr, sizeof(tr))) {
                    return -EFAULT;
                }
                ptr += sizeof(tr);
                binder_transaction(proc, thread, &tr
                    , cmd == BC_REPLY, 0);
                break;
            }
        }
    }

    return 0;
}

static struct binder_thread* binder_get_thread_ilocked(
        struct binder_proc* proc, struct binder_thread* new_thread) {
    struct binder_thread* thread = NULL;
    struct rb_node* parent = NULL;
    struct rb_node** p = &proc->threads.rb_node;
    pid_t pid;

#ifdef __WSL__
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
#ifdef __WSL__
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

static char* getIoctlCommandName(unsigned long cmd) {
    char* result = "undefined";
    switch (cmd) {
        case BINDER_WRITE_READ:
            result = "BINDER_WRITE_READ";
            break;
        case BINDER_SET_IDLE_TIMEOUT:
            result = "BINDER_SET_IDLE_TIMEOUT";
            break;
        case BINDER_SET_MAX_THREADS:
            result = "BINDER_SET_MAX_THREADS";
            break;
        case BINDER_SET_IDLE_PRIORITY:
            result = "BINDER_SET_IDLE_PRIORITY";
            break;
        case BINDER_SET_CONTEXT_MGR:
            result = "BINDER_SET_CONTEXT_MGR";
            break;
        case BINDER_THREAD_EXIT:
            result = "BINDER_THREAD_EXIT";
            break;
        case BINDER_VERSION:
            result = "BINDER_VERSION";
            break;
        case BINDER_GET_NODE_DEBUG_INFO:
            result = "BINDER_GET_NODE_DEBUG_INFO";
            break;
        case BINDER_GET_NODE_INFO_FOR_REF:
            result = "BINDER_GET_NODE_INFO_FOR_REF";
            break;
        case BINDER_SET_CONTEXT_MGR_EXT:
            result = "BINDER_SET_CONTEXT_MGR_EXT";
            break;
        default:
            break;
    }
    return result;
}

static int binder_ioctl_write_read(
    struct file* file, unsigned int cmd, unsigned long arg
            , struct binder_thread* thread) {
    int ret = 0;
    struct binder_proc* proc = file->private_data;
    unsigned int size = _IOC_SIZE(cmd);
    void __user* ubuf = (void __user*)arg;
    struct binder_write_read bwr;

    if (size != sizeof(struct binder_write_read)) {
        ret = -EINVAL;
        goto out;
    }
    if (copy_from_user(&bwr, ubuf, sizeof(bwr))) {
        ret = -EFAULT;
        goto out;
    }
    debug_binder(BINDER_DEBUG_WRITE_READ
                 , "%s: %d:%d, write %lld at %016llx"
                   ", read %lld at %016llx\n"
                 , __FUNCTION__, proc->pid
#ifdef __WSL__
                 , task_pid_vnr(current)
#else
                 , current->pid
#endif
                 , (u64)bwr.write_size, (u64)bwr.write_buffer
                 , (u64)bwr.read_size, (u64)bwr.read_buffer);

    if (bwr.write_size > 0) {
        ret = binder_thread_write(
                proc, thread
                , bwr.write_buffer
                , bwr.write_size
                , &bwr.write_consumed);
        // todo(20220419-143010 trace)
        if (ret < 0) {
            bwr.read_consumed = 0;
            if (copy_to_user(ubuf, &bwr, sizeof(bwr))) {
                ret = -EFAULT;
            }
            goto out;
        }
    }
    // todo(20220505-144737 if (bwr.read_size > 0))

    debug_binder(BINDER_DEBUG_WRITE_READ
                 , "%s: %d:%d, wrote %lld of %lld"
                   ", read return %lld of %lld\n"
                 , __FUNCTION__, proc->pid
#ifdef __WSL__
                 , task_pid_vnr(current)
#else
                 , current->pid
#endif
                 , (u64)bwr.write_consumed, (u64)bwr.write_size
                 , (u64)bwr.read_consumed, (u64)bwr.write_size);
    if (copy_to_user(ubuf, &bwr, sizeof(bwr))) {
        ret = -EFAULT;
        goto out;
    }

out:
    return ret;
}

static int binder_ioctl_set_ctx_mgr(
        struct file* file, struct flat_binder_object* fbo) {
    int ret = 0;
    struct binder_proc* proc = file->private_data;
    struct binder_context* context = proc->context;
    struct binder_node* new_node;
    // todo(20220427-160607 to consider in WSL, pid...)
    kuid_t curr_euid = current_euid();

    mutex_lock(&context->context_mgr_node_lock);
    if (context->binder_context_mgr_node) {
        pr_err("BINDER_SET_CONTEXT_MGR already set\n");
        ret = -EBUSY;
        goto out;
    }
    // todo(20220428-085816 selinux security)
    // ERROR: modpost: undefined!
    /*ret = security_binder_set_context_mgr(proc->tsk);
    if (ret < 0) {
        goto out;
    }*/
    // todo(20220428-085954 make a graphic uid)
    if (uid_valid(context->binder_context_mgr_uid)) {
        if (!uid_eq(context->binder_context_mgr_uid, curr_euid)) {
            pr_err("BINDER_SET_CONTEXT_MGR bad uid %d != %d\n"
                   , from_kuid(&init_user_ns, curr_euid)
                   , from_kuid(&init_user_ns
                   , context->binder_context_mgr_uid));
            ret = -EPERM;
            goto out;
        }
    } else {
        context->binder_context_mgr_uid = curr_euid;
    }
    new_node = binder_new_node(proc, fbo);
    if (!new_node) {
        ret = -EPERM;
        goto out;
    }
    binder_node_lock(new_node);
    new_node->local_weak_refs++;
    new_node->local_strong_refs++;
    new_node->has_weak_ref = 1;
    new_node->has_strong_ref = 1;
    context->binder_context_mgr_node = new_node;
    binder_node_unlock(new_node);
    binder_put_node(new_node);
out:
    mutex_unlock(&context->context_mgr_node_lock);
    return ret;
}

static long binder_ioctl(
        struct file* file, unsigned int cmd, unsigned long arg) {
    int ret;
    struct  binder_proc* proc = file->private_data;
    struct binder_thread* thread;
    unsigned int size = _IOC_SIZE(cmd);
    void* __user ubuf = (void* __user)arg;

    debug_binder(BINDER_DEBUG_OPEN_CLOSE
                 , "%s: %d:%d, cmd(%x) = %s, arg = %lx\n"
                 , __FUNCTION__
                 , proc->pid
#ifdef __WSL__
                 , task_pid_vnr(current)
#else
                 , current->pid
#endif
                 , cmd, getIoctlCommandName(cmd), arg);

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
        case BINDER_WRITE_READ: {
            ret = binder_ioctl_write_read(file, cmd, arg, thread);
            if (ret) {
                goto err;
            }
            break;
        }

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

        case BINDER_SET_CONTEXT_MGR_EXT: {
            struct flat_binder_object fbo;

            if (copy_from_user(&fbo, ubuf, sizeof(fbo))) {
                ret = -EINVAL;
                goto err;
            }

            ret = binder_ioctl_set_ctx_mgr(file, &fbo);
            if (ret) {
                goto err;
            }
            break;
        }

        case BINDER_SET_CONTEXT_MGR:
            ret = binder_ioctl_set_ctx_mgr(file, NULL);
            if (ret) {
                goto err;
            }
            break;

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
#ifdef __WSL__
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

#ifndef KBUILD_MODFILE
#define KBUILD_MODFILE "binder.c"
#endif // KBUILD_MODFILE

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("xiexuyuyan, <2351783158@qq.com>");
MODULE_DESCRIPTION("droid Binder");