#ifndef UTIL_MAYBEBOOL_H
#define UTIL_MAYBEBOOL_H

// Optional or "nullable" Boolean type (three-valued logic type)
typedef enum {
    MB_FALSE = 0,
    MB_TRUE = 1,
    MB_INDETERMINATE = 2,
} MaybeBool;

#endif
