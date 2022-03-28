#include "utils/Vector.h"

#include "log/log.h"

#undef TAG
#define TAG "VectorTest.cpp"

namespace droid {

    void testInsert() {
        LOG_D(TAG, "testInsert: ");
        Vector<int> vector;
        int a = 3;
        int b = 5;
        vector.insertAt(a, 0, 1);
        vector.insertAt(b, 1, 1);
    }
}


int main() {

    LOG_D(TAG, "main: start");

    droid::testInsert();

    LOG_D(TAG, "main: end\n\n\n");
}