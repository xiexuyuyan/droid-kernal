#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/list_lru.h>

extern struct list_lru mLruList;

enum lru_status lruFreePageEntry(
        struct list_head* item
        , struct list_lru_one* lru
        , spinlock_t* lock
        , void* cb_arg) __must_hold(lock) {
    pr_info("lruFreePageEntry: into\n");
    return LRU_SKIP;
}

static unsigned long lruCounter(
        struct shrinker* shrink, struct shrink_control* sc) {
    unsigned long ret;

    ret = list_lru_count(&mLruList);

    pr_info("lruCounter(&mLruList) = %lu \n", ret);

    return SHRINK_EMPTY;
}

static unsigned long lruScanner(
        struct shrinker* shrink, struct shrink_control* sc) {
    unsigned long ret;

    pr_info("lruScanner: into\n");

    ret = list_lru_walk(
            &mLruList
            , lruFreePageEntry
            , NULL
            , sc->nr_to_scan);

    pr_info("lruScanner: after lru list walk, ret = %lu\n", ret);

    return ret;
}

static struct shrinker mLruShrinker = {
        .count_objects = lruCounter,
        .scan_objects = lruScanner,
        .seeks = DEFAULT_SEEKS,
};

int registerLruShrinker(void) {
    return register_shrinker(&mLruShrinker);
}

void unregisterLruShrinker(void) {
    unregister_shrinker(&mLruShrinker);
}
