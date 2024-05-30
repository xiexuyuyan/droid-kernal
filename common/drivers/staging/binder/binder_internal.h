#ifndef _LINUX_BINDER_INTERNAL_H
#define _LINUX_BINDER_INTERNAL_H

#include <linux/export.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/refcount.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/uidgid.h>
#include <uapi/linux/android/binderfs.h>
#include "binder_alloc.h"

extern const struct file_operations binder_fops;

extern char* binder_devices_param;

extern bool is_binderfs_device(const struct inode* inode);

extern int __init init_binderfs(void);

enum binder_stat_types {
    BINDER_STAT_PROC,
    BINDER_STAT_THREAD,
    BINDER_STAT_NODE,
    BINDER_STAT_REF,
    BINDER_STAT_DEATH,
    BINDER_STAT_TRANSACTION,
    BINDER_STAT_TRANSACTION_COMPLETE,
    BINDER_STAT_COUNT
};

struct binder_stats {
    atomic_t bc[_IOC_NR(BC_REPLY_SG) + 1];
    atomic_t obj_created[BINDER_STAT_COUNT];
    atomic_t obj_deleted[BINDER_STAT_COUNT];
};

struct binderfs_mount_opts {
    int max;
    int stats_mode;
    // todo(15. what means stats_mode)
};

struct binderfs_info {
    struct ipc_namespace* ipc_ns;
    struct dentry* control_dentry;
    kuid_t root_uid;
    kgid_t root_gid;
    struct binderfs_mount_opts mount_opts;
    int device_count;
    struct dentry* proc_log_dir;
};


struct binder_work {
    struct list_head entry;

    enum binder_work_type {
        BINDER_WORK_TRANSACTION = 1,
        BINDER_WORK_TRANSACTION_COMPLETE,
        BINDER_WORK_TRANSACTION_ONEWAY_SPAM_SUSPECT,
        BINDER_WORK_RETURN_ERROR,
        BINDER_WORK_NODE,
        BINDER_WORK_DEAD_BINDER,
        BINDER_WORK_DEAD_BINDER_AND_CLEAR,
    } type;
};

struct binder_error {
    struct binder_work work;
    uint32_t cmd;
};

struct binder_priority {
    unsigned int sched_policy;
    int prio;
};

struct binder_proc {
    // todo(6. struct binder_proc)
    struct hlist_node proc_node;
    struct rb_root threads;
    struct rb_root nodes;
    int pid;
    struct task_struct* tsk;
    struct list_head waiting_threads;
    wait_queue_head_t  freeze_wait;

    struct list_head todo;
    struct binder_stats stats;
    struct list_head delivered_death;
    int max_threads;
    int tmp_ref;
    struct binder_priority default_priority;
    struct binder_alloc alloc;
    struct binder_context* context;
    spinlock_t inner_lock;
    spinlock_t outer_lock;
};

struct binder_node {
    int debug_id;
    spinlock_t lock;
    struct binder_work work;
    union {
        struct rb_node rb_node;
        struct hlist_node dead_node;
    };
    struct binder_proc* proc;
    struct hlist_head refs;
    int internal_strong_refs;
    int local_weak_refs;
    int local_strong_refs;
    int tmp_refs;
    binder_uintptr_t ptr;
    binder_uintptr_t cookie;
    struct {
        u8 has_strong_ref:1;
        u8 pending_strong_ref:1;
        u8 has_weak_ref:1;
        u8 pending_weak_ref:1;
    };
    struct {
        u8 sched_policy:2;
        u8 inherit_rt:1;
        u8 accept_fds:1;
        u8 txn_security_ctx:1;
        u8 min_priority;
    };
    struct list_head async_todo;
    // todo(7. struct binder_node{})
};

struct binder_context {
    struct binder_node* binder_context_mgr_node;
    struct mutex context_mgr_node_lock;
    kuid_t binder_context_mgr_uid;
    // todo(5. struct binder_node)
    const char* name;
};

struct binder_device {
    struct hlist_node hlist;
    // todo(2. what is hlist_node?)
    struct miscdevice miscdev;
    struct binder_context context;
    // todo(3. struct binder_context)
    struct inode* binderfs_inode;
    // todo(4. what is inode's using in binder driver?)
    refcount_t ref;
};

struct binder_transaction_log_entry {
    int debug_id;
    int debug_id_done;
    int call_type;
    int from_proc;
    int from_thread;
    int target_handle;
    int to_proc;
    int to_thread;
    int to_node;
    int data_size;
    int offsets_size;
    int return_error_line;
    uint32_t return_error;
    uint32_t return_error_param;
    char context_name[BINDERFS_MAX_NAME + 1];
};

struct binder_transaction_log {
    atomic_t cur;
    bool full;
    struct binder_transaction_log_entry entry[32];
};

struct binder_thread {
    struct binder_proc* proc;
    struct rb_node rb_node;
    struct list_head waiting_thread_node;
    int pid;
    int looper;
    bool looper_need_return;
    struct binder_transaction* transaction_stack;
    struct list_head todo;
    struct binder_error return_error;
    struct binder_error reply_error;
    wait_queue_head_t wait;
    struct binder_stats stats;
    atomic_t tmp_ref;
    struct task_struct* task;
};

struct binder_transaction {
    int debug_id;
    struct binder_work work;
    struct binder_thread *from;
    struct binder_transaction *from_parent;
    struct binder_proc *to_proc;
    struct binder_thread *to_thread;
    struct binder_transaction *to_parent;
    unsigned need_reply:1;
    /* unsigned is_dead:1; */       /* not used at the moment */

    struct binder_buffer *buffer;
    unsigned int    code;
    unsigned int    flags;
    struct binder_priority priority;
    struct binder_priority saved_priority;
    bool set_priority_called;
    bool is_nested;
    kuid_t  sender_euid;
    struct list_head fd_fixups;
    binder_uintptr_t security_ctx;
    /**
    * @lock:  protects @from, @to_proc, and @to_thread
    *
    * @from, @to_proc, and @to_thread can be set to NULL
    * during thread teardown
    */
    spinlock_t lock;
};

#endif // _LINUX_BINDER_INTERNAL_H