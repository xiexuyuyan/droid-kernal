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

static inline struct vm_area_struct* binder_alloc_get_vma(
        struct binder_alloc* alloc) {
    struct vm_area_struct* vma = NULL;

    if (alloc->vma) {
        smp_rmb();
        vma = alloc->vma;
    }

    return vma;
}

enum lru_status binder_alloc_free_page(
        struct list_head* item
        , struct list_lru_one* lru
        , spinlock_t* lock
        , void* cb_arg) __must_hold(lock) {
    struct mm_struct* mm = NULL;
    // from this statement, we implicate @item is a member
    // in struct @binder_lru_page named struct list_head @lru.
    struct binder_lru_page* page = container_of(
            item, struct binder_lru_page, lru);
    struct binder_alloc* alloc;
    uintptr_t page_addr;
    size_t index;
    struct vm_area_struct* vma;

    alloc = page->alloc;

    if (!mutex_trylock(&alloc->mutex))
        goto err_get_alloc_mutex_failed;

    if (!page->page_ptr)
        goto err_page_already_freed;

    // we have known that alloc->pages is an array pointed to
    // an addr allocate by kcalloc(num, size, flags),
    // which size = sizeof(struct binder_lru_page),
    // I guess it = 1 when make a sub operation.
    index = page - alloc->pages;
    page_addr = (uintptr_t)alloc->buffer + index * PAGE_SIZE;

    mm = alloc->vma_vm_mm;
    // increase the member of mm_struct, mm_users
    if (!mmget_not_zero(mm))
        goto err_mmget;
    if (!down_read_trylock(&mm->mmap_lock))
        goto err_down_read_mmap_sem_failed;
    vma = binder_alloc_get_vma(alloc);

    list_lru_isolate(lru, item);
    spin_unlock(lock);

    if (vma) {
        // todo(trace)
        zap_page_range(vma, page_addr, PAGE_SIZE);
    }
    up_read(&mm->mmap_lock);
    mmput_async(mm);

    // todo(trace)
    __free_page(page->page_ptr);
    page->page_ptr = NULL;

    spin_lock(lock);
    mutex_unlock(&alloc->mutex);
    return LRU_REMOVED_RETRY;

err_down_read_mmap_sem_failed:
    mmput_async(mm);
err_mmget:
err_page_already_freed:
    mutex_unlock(&alloc->mutex);
err_get_alloc_mutex_failed:
    return LRU_SKIP;
}


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
    unsigned long ret;

    pr_info("into %s.\n", __FUNCTION__ );

    ret = list_lru_walk(
            &binder_alloc_lru
            , binder_alloc_free_page
            , NULL
            , sc->nr_to_scan);


    return ret;
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