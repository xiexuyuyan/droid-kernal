#ifndef DROID_PROCESS_STATE_H
#define DROID_PROCESS_STATE_H

#include "utils/RefBase.h"
#include "utils/Mutex.h"

#include "utils/String8.h"

namespace droid {

    class ProcessState: public virtual RefBase {
    public:
        static sp<ProcessState> self();

    private:
        explicit ProcessState(const char* driver);
        ~ProcessState() override;

        ProcessState& operator=(const ProcessState& o);

        String8 mDriverName;
        int mDriverFD;
        void* mVMStart;
    };

} // namespace droid

#endif // DROID_PROCESS_STATE_H