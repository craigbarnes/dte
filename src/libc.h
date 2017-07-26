#ifndef LIBC_H
#define LIBC_H

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <dirent.h>
#include <pwd.h>
#include <time.h>
#include <wctype.h>
#include <signal.h>
#include <iconv.h>
#include <stdbool.h>

#if defined(__GNUC__)
#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)
#define NORETURN	__attribute__((__noreturn__))
#define FORMAT(idx)	__attribute__((format(printf, (idx), (idx + 1))))
#else
#define likely(x)	(x)
#define unlikely(x)	(x)
#define NORETURN
#define FORMAT(idx)
#endif

#define ARRAY_COUNT(x) ((unsigned long)sizeof(x) / sizeof(x[0]))
#define clear(ptr) memset((ptr), 0, sizeof(*(ptr)))

#endif
