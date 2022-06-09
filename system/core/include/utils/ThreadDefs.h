#ifndef DROID_UTILS_THREAD_DEFS_H
#define DROID_UTILS_THREAD_DEFS_H

#include <sys/resource.h>

typedef void* droid_thread_id_t;
typedef int (*droid_thread_func_t)(void*);

typedef droid_thread_id_t thread_id_t;
typedef droid_thread_func_t thread_func_t;

enum {
    PRIORITY_DEFAULT        = 19,
};

#endif // DROID_UTILS_THREAD_DEFS_H
