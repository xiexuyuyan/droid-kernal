#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/compiler_types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/fsnotify.h>
#include <linux/gfp.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/ipc_namespace.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/namei.h>
#include <linux/magic.h>
#include <linux/major.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/mount.h>
#include <linux/parser.h>
#include <linux/radix-tree.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/spinlock_types.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/user_namespace.h>
#include <linux/xarray.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/android/binder.h>
#include <uapi/linux/android/binderfs.h>

#include "binder_internal.h"

#define BINDERFS_MAX_MINOR (1u << MINORBITS)

static dev_t binderfs_dev;

static struct dentry* binderfs_mount(
        struct file_system_type* fs_type
        , int flags
        , const char* dev_name
        , void* data) {
    return NULL;
}

static void binderfs_kill_super(struct super_block* sb) {

}

static struct file_system_type binderfs_type = {
        .name     = "binder",
        .mount    = binderfs_mount,
        .kill_sb  = binderfs_kill_super,
        .fs_flags = FS_USERNS_MOUNT,
};

int __init init_binderfs(void) {
    int ret;
    const char* name;
    size_t len;

    name = binder_devices_param;
    for (len = strcspn(name, ","); len > 0; len = strcspn(name, ",")) {
        if (len > BINDERFS_MAX_NAME)
            return -E2BIG;
        name += len;
        if (*name == ',')
            name++;
    }

    ret = alloc_chrdev_region(&binderfs_dev, 0
                              , BINDERFS_MAX_MINOR, "binder");
    if (ret)
        return ret;

    ret = register_filesystem(&binderfs_type);
    if (ret) {
        unregister_chrdev_region(binderfs_dev, BINDERFS_MAX_MINOR);
        return ret;
    }

    return ret;
}