#include <mutex>
#include "binder/IServiceManager.h"

#include "droid/os/IServiceManager.h"
#include "binder/IPCThreadState.h"

namespace droid {
    using IdlServiceManager = droid::os::IServiceManager;

    [[clang::no_destroy]] static std::once_flag gSmOnce;
    // todo(20220324-125707 what means no_destroy? once_flag? call_once?)
    sp<IServiceManager> defaultServiceManager() {
        std::call_once(gSmOnce, []() {
            sp<IdlServiceManager> sm = nullptr;
            while(sm == nullptr) {
                sm = interface_cast<IdlServiceManager>(
                        ProcessState::self()->getContextObject(nullptr));
            }
        });


        return nullptr;
    }
} // namespace droid