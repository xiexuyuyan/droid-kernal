#include <cstring>
#include "log/log.h"
#include "utils/String8.h"

#undef TAG
#define TAG "String8Test.cpp"

namespace droid {
    static void testEqual() {
        String8 tmp("hello world");
        int compare = strcmp("hello world", tmp.string());
        LOGF_ASSERT(!compare, "testEqual: not equal");
    }
}
int main() {
    LOGF_D(TAG, "main: start");
    droid::testEqual();

    LOGF_D(TAG, "main: uint8_t* = %d", sizeof(uint8_t*));
    LOGF_D(TAG, "main: void* = %d", sizeof(void*));
    LOGF_D(TAG, "main: end\n\n\n");
}