#ifndef _LINUX_BINDER_ALLOC_H
#define _LINUX_BINDER_ALLOC_H

#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/rtmutex.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/list_lru.h>
#include <uapi/linux/android/binder.h>

extern void mmput_async(struct mm_struct *mm);

struct binder_buffer {
    struct list_head entry;
    struct rb_node rb_node;
    unsigned free:1;
    unsigned clear_on_free:1;
    unsigned allow_user_free:1;
    unsigned async_transaction:1;
    unsigned debug_id:28;

    void* __user user_data;
};

struct binder_lru_page {
    struct list_head lru;
    struct page* page_ptr;
    struct binder_alloc* alloc;
};

struct binder_alloc {
    struct mutex mutex;
    struct vm_area_struct* vma;
    struct mm_struct* vma_vm_mm;
    void __user* buffer;
    struct list_head buffers;
    struct rb_root free_buffers;
    struct rb_root allocated_buffers;
    size_t free_async_space;
    struct binder_lru_page* pages;
    size_t buffer_size;
    uint32_t buffer_free;
    int pid;
    size_t pages_high;
};

extern void binder_alloc_init(struct binder_alloc* alloc);
extern int binder_alloc_shrinker_init(void);
extern void binder_alloc_vma_close(struct binder_alloc* alloc);
extern int binder_alloc_mmap_handler(struct binder_alloc* alloc
        , struct vm_area_struct* vma);
extern void binder_alloc_exit(void);

#endif //_LINUX_BINDER_ALLOC_H