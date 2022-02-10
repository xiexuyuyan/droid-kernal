#include "log/log.h"

#include "utils/RefBase.h"
#include "utils/StrongPointer.h"

class LightClass : public droid::LightRefBase<LightClass> {
public:
    LightClass() {
        LOG_D("LightClass" ,"Constructor in Light class");
    }
    virtual ~LightClass() {
        LOG_D("LightClass", "Destroy in Light class");
    }
};

int main() {
    LOG_D("LightPointerTest", "start in light pointer test");
    {

        LightClass *pLightClass = new LightClass();

        LOG_D("LightPointerTest"
                , "Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount()));

        droid::sp<LightClass> spLightClass = pLightClass;

        LOG_D("LightPointerTest"
                , "Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount()));

        {
            droid::sp<LightClass> spLightClass1 = pLightClass;
            LOG_D("LightPointerTest"
                    , "Light Ref count is "
                      + std::to_string(pLightClass->getStrongCount()));
        }

        LOG_D("LightPointerTest"
                , "Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount()));

    }
    LOG_D("LightPointerTest", "end in light pointer test");
    return 0;
}