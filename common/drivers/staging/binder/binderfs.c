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
#include "binder_fs.h"

#define FIRST_INODE 1
#define SECOND_INODE 2
#define INODE_OFFSET 3

static DEFINE_MUTEX(binderfs_minors_mutex);
static DEFINE_IDA(binderfs_minors);

#define BINDERFS_MAX_MINOR (1u << MINORBITS)
#define BINDERFS_MAX_MINOR_CAPPED (BINDERFS_MAX_MINOR - 4)

static dev_t binderfs_dev;

enum {
    Opt_max,
    Opt_stat_mode,
    Opt_err
};

enum binderfs_stats_mode {
    STATS_NONE,
    STATS_GLOBAL,
};

static const match_table_t tokens = {
        {Opt_max, "max=%d"},
        {Opt_stat_mode, "stats=%s"},
        {Opt_err, NULL}
};

static inline struct binderfs_info* BINDERFS_I(const struct inode* inode) {
    return inode->i_sb->s_fs_info;
}

static int binderfs_binder_device_create(
        struct inode* ref_inode
        , struct binderfs_device __user* userp
        , struct binderfs_device* req) {
    int minor, ret;
    struct dentry* dentry, * root;
    struct binder_device* device;
    char* name = NULL;
    size_t name_len;
    struct inode* inode = NULL;
    struct super_block* sb = ref_inode->i_sb;
    struct binderfs_info* info = sb->s_fs_info;
#if defined(CONFIG_IPC_NS)
    bool use_reserve = (info->ipc_ns == &init_ipc_ns);
#else
    bool use_reserve = true;
#endif

    mutex_lock(&binderfs_minors_mutex);
    if (++info->device_count <= info->mount_opts.max) {
        minor = ida_alloc_max(
                &binderfs_minors,
                use_reserve ? BINDERFS_MAX_MINOR :
                    BINDERFS_MAX_MINOR_CAPPED,
                GFP_KERNEL);
    } else {
        minor = -ENOSPC;
    }
    if (minor < 0) {
        --info->device_count;
        mutex_unlock(&binderfs_minors_mutex);
        return minor;
    }
    mutex_unlock(&binderfs_minors_mutex);

    ret = -ENOMEM;
    device = kzalloc(sizeof(*device), GFP_KERNEL);
    if (!device)
        goto err;

    inode = new_inode(sb);
    if (!inode)
        goto err;

    inode->i_ino = minor + INODE_OFFSET;
    // todo(14. what means for i_ino to plus INODE_OFFSET)
    inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
    init_special_inode(inode, S_IFCHR | 0600,
                       MKDEV(MAJOR(binderfs_dev), minor));
    inode->i_fop = &binder_fops;
    inode->i_uid = info->root_uid;
    inode->i_gid = info->root_gid;

    req->name[BINDERFS_MAX_NAME] = '\0';
    name_len = strlen(req->name);
    name = kmemdup(req->name, name_len + 1, GFP_KERNEL);
    if (!name)
        goto err;

    refcount_set(&device->ref, 1);
    device->binderfs_inode = inode;
    device->context.binder_context_mgr_uid = INVALID_UID;
    device->context.name = name;
    device->miscdev.name = name;
    device->miscdev.minor = minor;
    mutex_init(&device->context.context_mgr_node_lock);

    req->major = MAJOR(binderfs_dev);
    req->minor = minor;

    if (userp && copy_to_user(userp, req, sizeof(*req))) {
        ret = -EFAULT;
        goto err;
    }

    root = sb->s_root;
    inode_lock(d_inode(root));

    dentry = lookup_one_len(name, root, name_len);
    if (IS_ERR(dentry)) {
        inode_unlock(d_inode(root));
        ret = PTR_ERR(dentry);
        goto err;
    }

    if (d_really_is_positive(dentry)) {
        dput(dentry);
        inode_unlock(d_inode(root));
        ret = -EEXIST;
        goto err;
    }

    inode->i_private = device;
    d_instantiate(dentry, inode);
    fsnotify_create(root->d_inode, dentry);
    inode_unlock(d_inode(root));

    return 0;

err:
    kfree(name);
    kfree(device);
    mutex_lock(&binderfs_minors_mutex);
    --info->device_count;
    ida_free(&binderfs_minors, minor);
    mutex_unlock(&binderfs_minors_mutex);
    iput(inode);

    return ret;
}



static void binderfs_evict_inode(struct inode* inode) {
    struct binder_device* device = inode->i_private;
    struct binderfs_info* info = BINDERFS_I(inode);

    clear_inode(inode);

    if (!S_ISCHR(inode->i_mode) || !device)
        return;

    mutex_lock(&binderfs_minors_mutex);
    --info->device_count;
    ida_free(&binderfs_minors, device->miscdev.minor);
    mutex_unlock(&binderfs_minors_mutex);

    if (refcount_dec_and_test(&device->ref)) {
        kfree(device->context.name);
        kfree(device);
    }
}

static int binderfs_parse_mount_opts(
        char* data, struct binderfs_mount_opts* opts) {
    char* p, * stats;
    opts->max = BINDERFS_MAX_MINOR;
    opts->stats_mode = STATS_NONE;

    while ((p = strsep(&data, ",")) != NULL) {
        substring_t args[MAX_OPT_ARGS];
        int token;
        int max_devices;

        if (!*p)
            continue;

        token = match_token(p, tokens, args);
        // todo(13. what means token?)
        switch (token) {
            case Opt_max:
                if (match_int(&args[0], &max_devices) ||
                        (max_devices < 0 ||
                            (max_devices > BINDERFS_MAX_MINOR)
                        )
                ) {
                    return -EINVAL;
                }
                opts->max = max_devices;
                break;

            case Opt_stat_mode:
                if (!capable(CAP_SYS_ADMIN))
                    return -EINVAL;

                stats = match_strdup(&args[0]);
                if (!stats)
                    return -ENOMEM;

                if (strcmp(stats, "global") != 0) {
                    kfree(stats);
                    return -EINVAL;
                }

                opts->stats_mode = STATS_GLOBAL;
                kfree(stats);
                break;
            default:
                pr_err("Invalid mount options\n");
                return -EINVAL;
        }
    }

    return 0;
}

static int binderfs_remount(struct super_block* sb, int* flag, char* data) {
    int prev_stats_mode, ret;
    struct binderfs_info* info = sb->s_fs_info;

    prev_stats_mode = info->mount_opts.stats_mode;
    ret = binderfs_parse_mount_opts(data, &info->mount_opts);
    if (ret)
        return ret;

    if (prev_stats_mode != info->mount_opts.stats_mode) {
        pr_err("Binder fs stats mode cannot be changed during a remount\n");
        info->mount_opts.stats_mode = prev_stats_mode;
        return -EINVAL;
    }

    return 0;
}

static int binderfs_show_mount_opts(struct seq_file* seq, struct dentry* root) {
    struct binderfs_info* info;

    info = root->d_sb->s_fs_info;
    if (info->mount_opts.max <= BINDERFS_MAX_MINOR) {
        seq_printf(seq, ",max=%d", info->mount_opts.max);
    }
    if (info->mount_opts.stats_mode == STATS_GLOBAL) {
        seq_printf(seq, ",stats=global");
    }

    return 0;
}

static const struct super_operations binderfs_super_ops = {
        .evict_inode = binderfs_evict_inode,
        .remount_fs = binderfs_remount,
        .show_options = binderfs_show_mount_opts,
        .statfs = simple_statfs,
};

static long binder_ctl_ioctl(
        struct file* file, unsigned int cmd, unsigned long arg) {
    int ret = -EINVAL;
    struct inode* inode = file_inode(file);
    struct binderfs_device __user* device =
            (struct binderfs_device __user* )arg;
    struct binderfs_device device_req;

    switch (cmd) {
        case BINDER_CTL_ADD:
            ret = copy_from_user(&device_req, device, sizeof(device_req));
            if (ret) {
                ret = -EFAULT;
                break;
            }
            ret = binderfs_binder_device_create(inode, device, &device_req);
            break;
        default:
            break;
    }

    return ret;
}

static const struct file_operations binder_ctl_fops = {
        .owner = THIS_MODULE,
        .open = nonseekable_open,
        .unlocked_ioctl = binder_ctl_ioctl,
        .compat_ioctl = binder_ctl_ioctl,
        .llseek = noop_llseek,
};

static int binderfs_binder_ctl_create(struct super_block* sb) {
    int minor, ret;
    struct dentry* dentry;
    struct binder_device* device;
    struct inode* inode;
    struct dentry* root = sb->s_root;
    struct binderfs_info* info = sb->s_fs_info;
#if defined(CONFIG_IPC_NS)
    bool use_reserve = (info->ipc_ns == &init_ipc_ns);
#else
    bool use_reserve = true;
#endif

    pr_info("into %s.\n", __FUNCTION__ );

    device = kzalloc(sizeof(*device), GFP_KERNEL);
    if (!device)
        return -ENOMEM;

    if (info->control_dentry) {
        ret = 0;
        goto out;
    }

    ret = -ENOMEM;
    inode = new_inode(sb);
    if (!inode)
        goto out;

    mutex_lock(&binderfs_minors_mutex);
    minor = ida_alloc_max(&binderfs_minors
            , use_reserve ? BINDERFS_MAX_MINOR : BINDERFS_MAX_MINOR_CAPPED
            , GFP_KERNEL);
    mutex_unlock(&binderfs_minors_mutex);
    if (minor < 0) {
        ret = minor;
        goto out;
    }

    inode->i_ino = SECOND_INODE;
    inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
    init_special_inode(inode, S_IFCHR | 0600, MKDEV(MAJOR(binderfs_dev), minor));
    inode->i_fop = &binder_ctl_fops;
    inode->i_uid = info->root_uid;
    inode->i_gid = info->root_gid;

    refcount_set(&device->ref, 1);
    device->binderfs_inode = inode;
    device->miscdev.minor = minor;

    dentry = d_alloc_name(root, "binder-control");
    if (!dentry)
        goto out;

    inode->i_private = device;
    info->control_dentry = dentry;
    d_add(dentry, inode);

    return 0;

out:
    kfree(device);
    iput(inode);

    return ret;
}

static const struct inode_operations binderfs_dir_inode_operations = {
        .lookup = simple_lookup,
        // todo(16. handle binderfs dir operations)
};

static int binderfs_fill_super(
        struct super_block* sb, void* data, int silent) {
    int ret;
    struct binderfs_info* info;
    struct inode* inode;
    struct binderfs_device device_info = { 0 };
    const char* name;
    size_t len;

    pr_info("into %s.\n", __FUNCTION__ );

    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;

    sb->s_iflags &= ~SB_I_NODEV;
    sb->s_iflags |= SB_I_NOEXEC;
    sb->s_magic = BINDERFS_SUPER_MAGIC;
    sb->s_op = &binderfs_super_ops;
    sb->s_time_gran = 1;

    sb->s_fs_info = kzalloc(sizeof(struct binderfs_info), GFP_KERNEL);
    if (!sb->s_fs_info)
        return -ENOMEM;
    info = sb->s_fs_info;

    ret = binderfs_parse_mount_opts(data, &info->mount_opts);
    if (ret)
        return ret;

    info->root_gid = make_kgid(sb->s_user_ns, 0);
    if (!gid_valid(info->root_gid))
        info->root_gid = GLOBAL_ROOT_GID;
    info->root_uid = make_kuid(sb->s_user_ns, 0);
    if (!uid_valid(info->root_uid))
        info->root_uid = GLOBAL_ROOT_UID;

    inode = new_inode(sb);
    if (!inode)
        return -ENOMEM;

    inode->i_ino = FIRST_INODE;
    inode->i_fop = &simple_dir_operations;
    inode->i_mode = S_IFDIR | 0755;
    inode->i_mtime = inode->i_atime = inode->i_ctime = current_time(inode);
    inode->i_op = &binderfs_dir_inode_operations;
    set_nlink(inode, 2);
    // todo(11. what is nlink? why set to 2?)

    sb->s_root = d_make_root(inode);
    if (!sb->s_root)
        return -ENOMEM;

    ret = binderfs_binder_ctl_create(sb);
    if (ret)
        return ret;

    name = binder_devices_param;
    for (len = strcspn(name, ","); len > 0; len = strcspn(name, ",")) {
        strscpy(device_info.name, name, len + 1);
        pr_info("auto mount %s", device_info.name);
        ret = binderfs_binder_device_create(inode, NULL, &device_info);
        if (ret)
            return ret;
        name += len;
        if (*name == ',')
            name++;
    }

    return 0;
}

static struct dentry* binderfs_mount(
        struct file_system_type* fs_type
        , int flags
        , const char* dev_name
        , void* data) {
    pr_info("into %s: mount device is %s.\n", __FUNCTION__, dev_name);
    return mount_nodev(fs_type, flags, data, binderfs_fill_super);
}

static void binderfs_kill_super(struct super_block* sb) {
    struct binderfs_info* info;
    pr_info("into %s.\n", __FUNCTION__ );

    info = sb->s_fs_info;

    kill_litter_super(sb);

    if (info && info->ipc_ns) {
        put_ipc_ns(info->ipc_ns);
        // todo(9. what is put_ipc_ns)
    }

    kfree(info);
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

void __exit binder_fs_exit(void) {
    pr_info("into %s.\n", __FUNCTION__ );
    unregister_filesystem(&binderfs_type);
    unregister_chrdev_region(binderfs_dev, BINDERFS_MAX_MINOR);
}
