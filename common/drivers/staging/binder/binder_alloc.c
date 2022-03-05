#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/list.h>
#include <linux/sched/mm.h>
#include <linux/module.h>
#include <linux/rtmutex.h>
#include <linux/rbtree.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/list_lru.h>
#include <linux/ratelimit.h>
#include <asm/cacheflush.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>
#include <linux/sizes.h>
#include "binder_alloc.h"
// #include "binder_trace.h"

struct list_lru binder_alloc_lru;

static unsigned long binder_shrink_count(
        struct shrinker* shrink, struct shrink_control* sc) {
    unsigned long ret;

    pr_info("into %s.\n", __FUNCTION__ );

    ret = list_lru_count(&binder_alloc_lru);

    pr_info("list_lru_count(&binder_alloc_lru) = %lu \n", ret);

    return SHRINK_EMPTY;
}
static unsigned long binder_shrink_scan(
        struct shrinker* shrink, struct shrink_control* sc) {
    pr_info("into %s.\n", __FUNCTION__ );
    return SHRINK_STOP;
}

static struct shrinker binder_shrinker = {
        .count_objects = binder_shrink_count,
        .scan_objects = binder_shrink_scan,
        .seeks = DEFAULT_SEEKS,
};

int binder_alloc_shrinker_init(void) {
    int ret = list_lru_init(&binder_alloc_lru);

    if (ret == 0) {
        ret = register_shrinker(&binder_shrinker);
        if (ret)
            list_lru_destroy(&binder_alloc_lru);
    }

    return ret;
}