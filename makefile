# This shim makefile directs non-GNU makes to run gmake
.POSIX:
.SUFFIXES:

all:
	+gmake -f GNUmakefile all

.DEFAULT:
	+gmake -f GNUmakefile $<
