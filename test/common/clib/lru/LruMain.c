#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list_lru.h>

extern int registerLruShrinker(void);
extern void unregisterLruShrinker(void);
struct list_lru mLruList;


static int __init lruInit(void) {
    int ret = -1;

    pr_info("lruInit: __init into\n");

    ret = list_lru_init(&mLruList);
    if (ret == 0) {
        ret = registerLruShrinker();
        if (ret != 0) {
            list_lru_destroy(&mLruList);
        }
    }

    pr_info("lruInit: after init ret = %d\n", ret);

    return 0;
}

static void __exit lruExit(void) {
    list_lru_destroy(&mLruList);
    unregisterLruShrinker();
    pr_info("lruExit: __exit into\n");
}


module_init(lruInit);
module_exit(lruExit);

#ifndef KBUILD_MODFILE
#define KBUILD_MODFILE "LruMain.c"
#endif // KBUILD_MODFILE
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("xiexuyuyan, <2351783158@qq.com>");
MODULE_DESCRIPTION("Lru example Linux module.");
MODULE_VERSION("0.01");
