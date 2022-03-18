#include "log/log.h"

#include "utils/RefBase.h"
#include "utils/StrongPointer.h"

#undef LOG_TAG
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
        LOG_D(LOG_TAG, std::string("Strong Ref Count = "
                       + std::to_string(strongValue)).c_str());
        LOG_D(LOG_TAG, std::string("Weak Ref Count = "
                       + std::to_string(weak)).c_str());
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

class WeakImpl : public Base {
public:
    WeakImpl() {
        extendObjectLifetime(OBJECT_LIFETIME_WEAK);
        LOG_D(LOG_TAG, "Weak Impl Constructor");
    }

    ~WeakImpl() override {
        LOG_D(LOG_TAG, "Weak Impl Destructor");
    }
};

void testStrongPinter() {
    LOG_D(LOG_TAG, "---------- strong impl start");
    {
        StrongImpl* strong = new StrongImpl();
        droid::wp<StrongImpl> wpStrong = droid::wp<StrongImpl>(strong);
        strong->printRefCount();
        {
            droid::sp<StrongImpl> spStrong = strong;
            strong->printRefCount();
        }
        LOG_D(LOG_TAG, "---------- attempt to promote");
        droid::sp<StrongImpl> spStrong = wpStrong.promote();
        char addr[20];
        std::sprintf(addr, "-%p-", spStrong.get());
        LOG_D(LOG_TAG, std::string("main: " + std::string(addr)).c_str());
    }
    LOG_D(LOG_TAG, "---------- strong impl end");
}

void testWeakPinter() {
    LOG_D(LOG_TAG, "---------- weak impl start");
    {
        WeakImpl* weak = new WeakImpl();
        droid::wp<WeakImpl> wpWeak = droid::wp<WeakImpl>(weak);
        weak->printRefCount();
        {
            droid::sp<WeakImpl> spWeak = weak;
            weak->printRefCount();
        }
        weak->printRefCount();
        LOG_D(LOG_TAG, "---------- attempt to promote");
        droid::sp<WeakImpl> spWeak = wpWeak.promote();
        weak->printRefCount();
        char addr[20];// too small will cause "*** stack smashing detected ***"
        std::sprintf(addr, "-%p-", spWeak.get());
        LOG_D(LOG_TAG, std::string("main: " + std::string(addr)).c_str());
    }
    LOG_D(LOG_TAG, "---------- weak impl end");
}

void testIncWeak() {
    LOG_D(LOG_TAG, "---------- weak impl start");
    {
        WeakImpl* weak = new WeakImpl();
        droid::wp<WeakImpl> wpWeak = droid::wp<WeakImpl>(weak);
        droid::sp<WeakImpl> spWeak = wpWeak.promote();
        weak->printRefCount();
    }
    LOG_D(LOG_TAG, "---------- weak impl end");
}

int main() {
    LOG_D(LOG_TAG, "hello world!");
    // testStrongPinter();
    testWeakPinter();
    // testIncWeak();
    return 0;
}