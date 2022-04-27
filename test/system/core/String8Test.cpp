#include <cstring>
#include "log/log.h"
#include "utils/String8.h"
#include "utils/String16.h"
#include "utils/Looper.h"

#undef TAG
#define TAG "String8Test.cpp"

namespace droid {
    static void testEqual() {
        String8 tmp("hello world");
        int compare = strcmp("hello world", tmp.string());
        LOGF_ASSERT(!compare, "testEqual: not equal");
    }

    static void testString16() {
        String16 string16("hello");
        LOGF_D(TAG, "testString16: %s", String8(string16).c_str());
    }
}

int main() {
    LOG_D(TAG, "main: start");

    LOG_D(TAG, "main: \n\n\n");
}
