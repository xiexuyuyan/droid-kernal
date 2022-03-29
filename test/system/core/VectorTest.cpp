#include <iostream>

#include "utils/Vector.h"
#include "utils/TypeHelpers.h"

#include "log/log.h"

#undef TAG
#define TAG "VectorTest.cpp"

namespace droid {
    namespace ShallowDeepCopyTest{
        template <typename TYPE>
        void testForEach_base_class_int(Vector<TYPE> vector) {
            int size = vector.size();
            int capacity = vector.capacity();
            const TYPE* head = vector.array();
            LOGF_D(TAG, "testInsert: size = %d, capacity = %d"
            , size, capacity);
            for (int i = 0; i < size; i++) {
                int* a = *(head++);
                LOGF_D(TAG, "testForEach: vector[%d] = %d", i, *a);
            }
        }

        void testShallowCopy_base_class_int() {
            LOG_D(TAG, "testInsert: ");
            Vector<int*> vector;
            int cap = vector.setCapacity(4);
            LOGF_D(TAG, "testInsert: cap = %d", cap);

            int a0 = 0;
            int a1 = 1;
            int a2 = 2;
            int a3 = 3;
            int a4 = 4;

            int* pa0 = &a0;
            int* pa1 = &a1;
            int* pa2 = &a2;
            int* pa3 = &a3;
            int* pa4 = &a4;

            vector.insertAt(pa0, 0, 1);
            vector.insertAt(pa1, 1, 1);
            vector.insertAt(pa2, 2, 1);
            vector.insertAt(pa3, 3, 1);
            testForEach_base_class_int(vector);
            vector.insertAt(pa4, 4, 1);
            testForEach_base_class_int(vector);

            a0 = 10086;
            testForEach_base_class_int(vector);
        }



        struct node {
            int value;
        };

        template<typename TYPE>
        void testForEach_Deep_Copy_struct_node_value(Vector<TYPE> vector) {
            int size = vector.size();
            int capacity = vector.capacity();
            const TYPE *head = vector.array();
            LOGF_D(TAG, "testInsert: size = %d, capacity = %d", size, capacity);
            for (int i = 0; i < size; i++) {
                struct node a = *(head++);
                LOGF_D(TAG, "testForEach: vector[%d] = %d", i, a.value);
            }
        }

        void testDeepCopy_struct_node_value() {
            LOG_D(TAG, "testInsert: ");
            Vector<struct node> vector;
            int cap = vector.setCapacity(4);
            LOGF_D(TAG, "testInsert: cap = %d", cap);

            struct node a0 = {0};
            struct node a1 = {1};
            struct node a2 = {2};
            struct node a3 = {3};
            struct node a4 = {4};

            vector.insertAt(a0, 0, 1);
            vector.insertAt(a1, 1, 1);
            vector.insertAt(a2, 2, 1);
            vector.insertAt(a3, 3, 1);
            testForEach_Deep_Copy_struct_node_value(vector);
            vector.insertAt(a4, 4, 1);
            testForEach_Deep_Copy_struct_node_value(vector);

            a0 = {10086};
            testForEach_Deep_Copy_struct_node_value(vector);

            struct node *pa0 = &a0;
            pa0->value = 10086;
            testForEach_Deep_Copy_struct_node_value(vector);
        }

        template<typename TYPE>
        void testForEach_Shallow_Copy_struct_node_value(Vector<TYPE> vector) {
            int size = vector.size();
            int capacity = vector.capacity();
            const TYPE *head = vector.array();
            LOGF_D(TAG, "testInsert: size = %d, capacity = %d", size, capacity);
            for (int i = 0; i < size; i++) {
                struct node *pa = *(head++);
                LOGF_D(TAG, "testForEach: vector[%d] = %d", i, pa->value);
            }
        }

        void testShallowCopy_struct_node_value() {
            LOG_D(TAG, "testInsert: ");
            Vector<struct node *> vector;
            int cap = vector.setCapacity(4);
            LOGF_D(TAG, "testInsert: cap = %d", cap);

            struct node a0 = {0};
            struct node a1 = {1};
            struct node a2 = {2};
            struct node a3 = {3};
            struct node a4 = {4};

            struct node *pa0 = &a0;
            struct node *pa1 = &a1;
            struct node *pa2 = &a2;
            struct node *pa3 = &a3;
            struct node *pa4 = &a4;

            vector.insertAt(pa0, 0, 1);
            vector.insertAt(pa1, 1, 1);
            vector.insertAt(pa2, 2, 1);
            vector.insertAt(pa3, 3, 1);
            testForEach_Shallow_Copy_struct_node_value(vector);
            vector.insertAt(pa4, 4, 1);
            testForEach_Shallow_Copy_struct_node_value(vector);

            a0 = {10086};
            testForEach_Shallow_Copy_struct_node_value(vector);

            // pa0->value = 10086;
            // testForEach_Shallow_Copy_struct_node_value(vector);
        }
    } // namespace ShallowDeepCopyTest


    template <typename TYPE>
    void testForEach(Vector<TYPE> vector) {
        int size = vector.size();
        int capacity = vector.capacity();
        const TYPE* head = vector.array();
        LOGF_D(TAG, "testInsert: size = %d, capacity = %d"
        , size, capacity);
        for (int i = 0; i < size; i++) {
            int a = *(head++);
            LOGF_D(TAG, "testForEach: vector[%d] = %d", i, a);
        }
    }

    void testBaseInt() {
        LOG_D(TAG, "testInsert: ");
        Vector<int> vector;
        int cap = vector.setCapacity(4);
        LOGF_D(TAG, "testInsert: cap = %d", cap);

        int a0 = 0;
        int a1 = 1;
        int a2 = 2;
        int a3 = 3;
        int a4 = 4;

        vector.insertAt(a0, 0, 1);
        vector.insertAt(a1, 1, 1);
        vector.insertAt(a2, 2, 1);
        vector.insertAt(a3, 3, 1);
        testForEach(vector);
        vector.insertAt(a4, 4, 1);
        testForEach(vector);

        vector.insertAt(10086, 0, 2);
        testForEach(vector);

    }

}


int main() {

    LOG_D(TAG, "main: start");

    droid::testBaseInt();

    LOG_D(TAG, "main: end\n\n\n");
}