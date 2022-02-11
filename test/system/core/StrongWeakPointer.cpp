#include "log/log.h"

#include "utils/RefBase.h"
#include "utils/StrongPointer.h"

#define LOG_TAG "StrongWeakPointer.cpp"

#define INITIAL_STRONG_VALUE (1<<28)

class Base : public droid::RefBase{
public:
    void printRefCount() {
        int32_t strong = getStrongCount();
        int strongValue = (strong == INITIAL_STRONG_VALUE) ? 0 : strong;
        weakref_type* ref = getWeakRefs();
        int32_t weak = ref->getWeakCount();

        LOG_D(LOG_TAG, "---------- printRefCount()");
        LOG_D(LOG_TAG, "Strong Ref Count = "
                       + std::to_string(strongValue));
        LOG_D(LOG_TAG, "Weak Ref Count = "
                       + std::to_string(weak));
        LOG_D(LOG_TAG, "---------- printRefCount()");
    }
};

class StrongImpl : public Base {
public:
    StrongImpl() {
        LOG_D(LOG_TAG, "Strong Impl Constructor");
    }

    ~StrongImpl() override {
        LOG_D(LOG_TAG, "Strong Impl Destructor");
    }
};


int main() {
    LOG_D(LOG_TAG, "hello world!");

    LOG_D(LOG_TAG, "---------- strong impl start");
    {
        StrongImpl* strong = new StrongImpl();
        droid::sp<StrongImpl> pStrong = strong;
        pStrong->printRefCount();
    }
    LOG_D(LOG_TAG, "---------- strong impl end");

    return 0;
}