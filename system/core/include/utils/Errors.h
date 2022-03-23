
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <string>

namespace droid {

/**
 * The type used to return success/failure from frameworks APIs.
 * See the anonymous enum below for valid values.
 */
typedef int32_t status_t;


enum {
    OK                = 0,    // Preferred constant for checking success.
    NO_ERROR          = OK,   // Deprecated synonym for `OK`. Prefer `OK` because it doesn't conflict with Windows.

    UNKNOWN_ERROR       = (-2147483647-1), // INT32_MIN value

    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = (UNKNOWN_ERROR + 1),
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = (UNKNOWN_ERROR + 2),

    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK, 
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
    
    FDS_NOT_ALLOWED     = (UNKNOWN_ERROR + 7),
    UNEXPECTED_NULL     = (UNKNOWN_ERROR + 8),
};

// Human readable name of error
std::string statusToString(status_t status);

}  // namespace droid
	