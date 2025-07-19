#ifndef UTIL_ERRORCODE_H
#define UTIL_ERRORCODE_H

#include <errno.h>

// Values of this type are either 0 (to indicate success) or an error
// code from <errno.h> (to indicate failure)
typedef int SystemErrno;

#endif
