C Library Feature Tests
=======================

## Synopsis

The files in this directory are used to detect support for extended
(non-standard) features in libc. Each `*.c` file corresponds to a
single feature to be detected and successful compilation is taken to
mean the feature is supported by the target platform.

## Adding New Tests

The following points should be observed when adding new feature tests:

* When detecting the presence of library functions, the function name
  should be enclosed in parenthesis, e.g. `(fsync)(1)`. This suppresses
  the expansion of function-like macros and forces the compiler to error
  out if there's no function declaration. (see https://ewontfix.com/13/).
* If the feature being detected depends on [`feature_test_macros(7)`], the
  required definitions should be added to [`defs.h`] and `#include "defs.h"`
  should be added at the top of the `*.c` file. The contents of [`defs.h`]
  is concatenated into `build/feature.h` by the build system, along with the
  generated headers from `build/feature/*.h`, to ensure the code under [`src/`]
  sees the same macro definitions as the feature tests.
* Source files under [`src/`] that use constructs like e.g. `#if HAVE_EXAMPLE`
  should use `#include "compat.h"` before any other includes and should be
  given an explicit dependency on `src/compat.h` in [`mk/build.mk`].
* Some platforms implement stubs for some extended functions, which simply
  fail at runtime and set [`errno`] to `ENOSYS`. See `xpipe2()` in
  [`src/util/fd.c`] for an example of how to handle this.


[`feature_test_macros(7)`]: https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
[`errno`]: https://man7.org/linux/man-pages/man3/errno.3.html

[`defs.h`]: defs.h
[`mk/build.mk`]: ../build.mk
[`src/`]: ../../src
[`src/util/fd.c`]: ../../src/util/fd.c
