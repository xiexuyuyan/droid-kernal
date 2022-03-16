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

char* binder_devices_param = "binder,hwbinder,vndbinder";

struct binder_transaction_log binder_transaction_log;
struct binder_transaction_log binder_transaction_log_failed;

const struct file_operations binder_fops = {
        .owner = THIS_MODULE
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