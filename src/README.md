dte source code
===============

This directory contains the `dte` source code. It makes liberal use
of ISO C99 features and POSIX 2008 APIs, but generally requires very
little else.

The main editor code is in the base directory and various other
(somewhat reusable) parts are in sub-directories:

* `editorconfig/` - [EditorConfig] implementation
* `encoding/` - charset encoding/decoding/conversion
* `filetype/` - filetype detection
* `syntax/` - syntax highlighting
* `terminal/` - terminal control and response parsing
* `util/` - data structures, string utilities, etc.

Header include paths
--------------------

There's a well known and ubiquitous problem with `#include` paths, caused
by the use of the `-I` flag common to most C compilers. This flag adds a
specific directory to both the local **and** system header lookup paths
and in doing so often causes naming clashes between the two. There are
various solutions to this problem:

1. "Namespace" all header files by using a filename prefix.
2. Use the `-iquote` option instead of `-I`.
3. Use `#include` paths that explicitly name the directory (relative to
   the source file) and avoid using the compiler flags entirely.
4. Put all source/header files in a single directory and sidestep the
   issue entirely.
5. Pretend the issue doesn't exist, or just hope that naming collisions
   don't happen by pure chance.

Out of these possible options, #1 generally works, but is extremely
ugly. #2 is clean and effective, but not at all portable. #3 is somewhat
verbose, but fully solves the problem while remaining portable. #4 is
unreasonably restrictive for a project with hundreds of files. #5 is
what a surprising number of projects seem to opt for, but is never a
sane choice.

The option used by dte is #3. This fact is mentioned here in order to
document the reason behind the unusual `#include` paths used by the
codebase and also to explicitly warn future developers about the perils
of using the `-I` flag, especially in conjunction with the current
header names.


[EditorConfig]: https://editorconfig.org/
