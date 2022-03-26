#include <mutex>
#include "binder/IServiceManager.h"

#include "droid/os/IServiceManager.h"
#include "binder/IPCThreadState.h"

#undef TAG
#define TAG "IServiceManager.cpp"

namespace droid {
    using IdlServiceManager = droid::os::IServiceManager;

    [[clang::no_destroy]] static std::once_flag gSmOnce;
    // todo(20220324-125707 what means no_destroy? once_flag? call_once?)
    sp<IServiceManager> defaultServiceManager() {
        LOG_D(TAG, "defaultServiceManager: ");
        std::call_once(gSmOnce, []() {
            sp<IdlServiceManager> sm = nullptr;

            sp<IBinder> impl =
                    ProcessState::self()->getContextObject(nullptr);
            // todo(20220326-130700 test get then test cast)
            /*while(sm == nullptr) {
                sm = interface_cast<IdlServiceManager>(
                        );
            }*/
        });


        return nullptr;
    }
} // namespace droid