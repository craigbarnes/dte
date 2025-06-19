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
  should be enclosed in parentheses, e.g. `(fsync)(1)`. This suppresses
  the expansion of function-like macros and forces the compiler to error
  out if there's no function declaration. (see https://ewontfix.com/13/).

*  If the feature being detected depends on `_GNU_SOURCE` being defined,
  `#include "defs.h"` should be added at the top of the `*.c` file. The
  contents of [`defs.h`] is concatenated into `build/gen/feature.h` by the
  build system, along with the generated headers from `build/feature/*.h`,
  to ensure the code under [`src/`] sees the same macro definitions as the
  feature tests. Most other platform-specific [`feature_test_macros(7)`]
  are defined by default (where applicable) and it's usually a mistake to
  explicitly define them (see commits [`a91a15760`] and [`38d9e95da`]).

* Source files under [`src/`] that use constructs like e.g. `#if HAVE_EXAMPLE`
  should use `#include "feature.h"` before any other includes and should be
  given an explicit dependency on `src/feature.h` in [`mk/build.mk`]. The
  `tools/hdrcheck.awk` script (and `make check-headers` target) will issue
  a warning, if this `#include` ordering isn't respected.

* Pre-processor `#if` or `#ifdef` guard blocks should be kept as small as
  is reasonably possible; ideally to just the single line where an extended
  feature is used (i.e. to avoid undefined symbol errors). Early `return`
  (and the resulting dead code elimination) should be used in all other
  cases, so that less code is hidden by these guard blocks and more code is
  checked by the compiler, regardless of which pre-processor defines are in
  effect.

* Some platforms implement stubs for some extended functions, which simply
  fail at runtime and set [`errno`] to `ENOSYS`. See `xpipe2()` in
  [`src/util/fd.c`] for an example of how to handle this.


[`feature_test_macros(7)`]: https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
[`a91a15760`]: https://gitlab.com/craigbarnes/dte/-/commit/a91a15760d145a1408d475d7ddb4044a733b6720
[`38d9e95da`]: https://gitlab.com/craigbarnes/dte/-/commit/38d9e95daa41484640c5cfe7d2080d9329c5789a
[`errno`]: https://man7.org/linux/man-pages/man3/errno.3.html

[`defs.h`]: defs.h
[`mk/build.mk`]: ../build.mk
[`src/`]: ../../src
[`src/util/fd.c`]: ../../src/util/fd.c
