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
    LOG_D("---------", "rall start");
    {
        LOG_D("---------", "start");
        LightClass* pLightClass = new LightClass();
        droid::sp<LightClass> spLightClassDirect = pLightClass;
        LOG_D("---------", "end");
    }
    LOG_D("---------", "rall end\n\n\n");


    LOG_D("---------", "cycle start");
    {
        LOG_D("---------", "start");
        LightClass* pLightClass = new LightClass();
        droid::sp<LightClass>* spLightClassDirect = new droid::sp<LightClass>(pLightClass);
        LOG_D("---------", "end");
    }
    LOG_D("---------", "cycle end\n\n\n");


    LOG_D("LightPointerTest", "start in light pointer test");
    {

        LightClass *pLightClass = new LightClass();

        LOG_D("LightPointerTest"
                , std::string("Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount())).c_str());

        droid::sp<LightClass> spLightClassDirect = pLightClass;
        LOG_D("LightPointerTest"
                , std::string("Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount())).c_str());

        {
            droid::sp<LightClass> spLightClassCopy(spLightClassDirect);
            LOG_D("LightPointerTest"
                    , std::string("Light Ref count is "
                      + std::to_string(pLightClass->getStrongCount())).c_str());
        }

        LOG_D("LightPointerTest"
                , std::string("Light Ref count is "
                  + std::to_string(pLightClass->getStrongCount())).c_str());

    }
    LOG_D("LightPointerTest", "end in light pointer test");
    return 0;
}