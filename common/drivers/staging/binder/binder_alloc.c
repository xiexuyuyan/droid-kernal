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

static DEFINE_MUTEX(binder_alloc_mmap_lock);

enum {
    BINDER_DEBUG_USER_ERROR         = 1U << 0,
    BINDER_DEBUG_BUFFER_ALLOC       = 1U << 2,
};
static uint32_t binder_alloc_debug_mask =
        BINDER_DEBUG_USER_ERROR
        | BINDER_DEBUG_BUFFER_ALLOC;
module_param_named(debug_mask_binder_alloc, binder_alloc_debug_mask, uint, 0644);

#define debug_binder_alloc(mask, x...) \
    do { \
        if(binder_alloc_debug_mask & mask) \
            pr_info_ratelimited(x); \
    } while(0)

static struct binder_buffer* binder_buffer_next(
        struct binder_buffer* buffer) {
    return list_entry(buffer->entry.next, struct binder_buffer, entry);
}

static size_t binder_alloc_buffer_size(
        struct binder_alloc* alloc
        , struct binder_buffer* buffer) {
    if (list_is_last(&buffer->entry, &alloc->buffers))
        // todo(20220323-185618 if into this case
        //  , it means we manual set @buffer->entry.next to @head)
        return alloc->buffer + alloc->buffer_size - buffer->user_data;
    return binder_buffer_next(buffer)->user_data - buffer->user_data;
}


static void binder_insert_free_buffer(
        struct binder_alloc* alloc
        , struct binder_buffer* new_buffer) {
    struct rb_node** p = &alloc->free_buffers.rb_node;
    struct rb_node* parent = NULL;
    struct binder_buffer* buffer;
    size_t buffer_size;
    size_t new_buffer_size;

    BUG_ON(!new_buffer->free);

    new_buffer_size = binder_alloc_buffer_size(alloc, new_buffer);

    debug_binder_alloc(BINDER_DEBUG_BUFFER_ALLOC
                       , "%s %d: add free buffer, size %zd, at %pK\n"
                       , __FUNCTION__
                       , alloc->pid, new_buffer_size, new_buffer);

    while(*p) {
        parent = *p;
        buffer = rb_entry(parent, struct binder_buffer, rb_node);
        BUG_ON(!buffer->free);

        buffer_size = binder_alloc_buffer_size(alloc, buffer);
        if (new_buffer_size < buffer_size)
            p = &parent->rb_left;
        else
            p = &parent->rb_right;
    }
    rb_link_node(&new_buffer->rb_node, parent, p);
    rb_insert_color(&new_buffer->rb_node, &alloc->free_buffers);
}

static inline void binder_alloc_set_vma(
        struct binder_alloc* alloc
        , struct vm_area_struct* vma) {
    if (vma)
        alloc->vma_vm_mm = vma->vm_mm;

    smp_wmb();
    alloc->vma = vma;
}

static inline struct vm_area_struct* binder_alloc_get_vma(
        struct binder_alloc* alloc) {
    struct vm_area_struct* vma = NULL;

    if (alloc->vma) {
        smp_rmb();
        vma = alloc->vma;
    }

    return vma;
}

int binder_alloc_mmap_handler(
        struct binder_alloc* alloc
        , struct vm_area_struct* vma) {
    int ret;
    const char* failure_string;
    struct binder_buffer* buffer;

    mutex_lock(&binder_alloc_mmap_lock);
    if (alloc->buffer_size) {
        ret = -EBUSY;
        failure_string = "already mapped";
        goto err_already_mapped;
    }
    alloc->buffer_size = min_t(unsigned long
            , vma->vm_end - vma->vm_start, SZ_4M);
    mutex_unlock(&binder_alloc_mmap_lock);

    alloc->buffer = (void* __user)vma->vm_start;

    alloc->pages = kcalloc(
            alloc->buffer_size / PAGE_SIZE
            , sizeof(alloc->pages[0]), GFP_KERNEL);
    if (alloc->pages == NULL) {
        ret = -ENOMEM;
        failure_string = "alloc page array";
        goto err_alloc_pages_failed;
    }

    buffer = kzalloc(sizeof(*buffer), GFP_KERNEL);
    if (!buffer) {
        ret = -ENOMEM;
        failure_string = "alloc buffer struct";
        goto err_alloc_buf_struct_failed;
    }

    buffer->user_data = alloc->buffer;
    list_add(&buffer->entry, &alloc->buffers);
    buffer->free = 1;
    binder_insert_free_buffer(alloc, buffer);
    alloc->free_async_space = alloc->buffer_size / 2;
    binder_alloc_set_vma(alloc, vma);
    mmgrab(alloc->vma_vm_mm);

    return 0;

err_alloc_buf_struct_failed:
    kfree(alloc->pages);
    alloc->pages = NULL;
err_alloc_pages_failed:
    alloc->buffer = NULL;
    mutex_lock(&binder_alloc_mmap_lock);
    alloc->buffer_size = 0;
err_already_mapped:
    mutex_unlock(&binder_alloc_mmap_lock);
    debug_binder_alloc(BINDER_DEBUG_USER_ERROR
                       , "%s: %d %lx-%lx %s failed %d\n"
                       , __FUNCTION__
                       , alloc->pid, vma->vm_start, vma->vm_end
                       , failure_string, ret);
    return ret;
}

void binder_alloc_vma_close(struct binder_alloc* alloc) {
    binder_alloc_set_vma(alloc, NULL);
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

    // we have known that @alloc->pages is an array pointed to
    // an addr allocate by kcalloc(num, size, flags),
    // Attention: lru saved pages which is marked as not-in-use
    // so, there we count not-in-use page's start addr by
    // add @alloc->buffer with num of in-use pages
    // |-use-|-use-|-lru1-|-lru2|
    // @pages      @page
    // @page is pointed to first lru page
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

void binder_alloc_init(struct binder_alloc* alloc) {
#ifdef __WSL__
    alloc->pid = task_tgid_vnr(current);
#else
    alloc->pid = current->group_leader->pid;
#endif
    mutex_init(&alloc->mutex);
    INIT_LIST_HEAD(&alloc->buffers);
}

int binder_alloc_shrinker_init(void) {
    int ret = list_lru_init(&binder_alloc_lru);

    if (ret == 0) {
        ret = register_shrinker(&binder_shrinker);
        if (ret)
            list_lru_destroy(&binder_alloc_lru);
    }

    return ret;
}

void binder_alloc_exit(void) {
    pr_info("into %s.\n", __FUNCTION__ );
    unregister_shrinker(&binder_shrinker);
    list_lru_destroy(&binder_alloc_lru);
}