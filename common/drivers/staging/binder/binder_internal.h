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

extern int __init init_binderfs(void);

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
        BINDER_WORK_NODE,
        BINDER_WORK_DEA_BINDER,
        BINDER_WORK_DEA_BINDER_AND_CLEAR,
        BINDER_WORK_CLEAR_DEATH_NOTIFICATION,
    } type;
};

struct binder_proc {
    // todo(6. struct binder_proc)
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

struct binder_transaction_log {
    atomic_t cur;
};

#endif // _LINUX_BINDER_INTERNAL_H