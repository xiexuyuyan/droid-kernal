#ifndef DROID_IBINDER_H
#define DROID_IBINDER_H

#include "utils/RefBase.h"

namespace droid {

    class [[clang::lto_visibility_public]] IBinder : public virtual RefBase {

    };

} // namespace droid
#endif // DROID_IBINDER_H