# These variables are used by all CI test runners.
# See also: `mk/prelude.mk` and `docs/packaging.md`

# Group build output by target, so that it doesn't get get interleaved
# (by parallel builds) in ways that make it hard to read.
# See also:
# • https://www.gnu.org/software/make/manual/make.html#index-_002d_002doutput_002dsync-1
# • https://www.gnu.org/software/make/manual/make.html#Parallel-Output:~:text=target,-Output%20from
MAKEFLAGS += -Otarget

DEBUG = 3 # Full debug assertions
WERROR = 1 # All compiler warnings are fatal
V = 1 # Verbose build output (print executed commands, instead of short messages)
TESTFLAGS = -ct # Enable colors and timing info in test runner output
