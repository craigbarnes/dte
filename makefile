# This shim makefile directs non-GNU makes to run gmake
.POSIX:
.SUFFIXES:

# Prevents bmake from tying to read ".depend" as a makefile
.MAKE.DEPENDFILE =

all:
	+gmake -f GNUmakefile all

.DEFAULT:
	+gmake -f GNUmakefile $<
