dte Tests
=========

This directory contains the automated testing infrastructure for dte.

File Overview
-------------

* `benchmark.c` - C source for the benchmark runner (`build/test/bench`)
* `*.c` - C sources for the test runner (`build/test/test`), excluding the above
* `main.c` - Test runner `main()` function (used instead of `src/main.c`)
* `test.h` - Assertion macros and related data structures
* `test.c` - Underlying code for assertion macros
* `data/*` - Various input files used by the test runner
* `check-opts.sh` - Script for testing [`dte(1)`] command-line options

Make Targets
------------

* `make check-tests` - Compile and execute the test runner
* `make check-opts` - Compile the editor and run `check-opts.sh` (see above)
* `make check` - Equivalent to the above 2 commands
* `make bench` - Compile and execute the benchmark runner
* `make help` - Print more information about available makefile targets


[`dte(1)`]: https://craigbarnes.gitlab.io/dte/dte.html
