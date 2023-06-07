dte source code
===============

This directory contains the `dte` source code. It makes liberal use
of ISO C99 features and [POSIX] 2008 APIs, but generally requires very
little else.

The main editor code is in the base directory and various other
(somewhat reusable) parts are in sub-directories:

* `command/` - command language parsing and execution
* `editorconfig/` - [EditorConfig] implementation
* `filetype/` - filetype detection
* `syntax/` - syntax highlighting
* `terminal/` - terminal input/output handling
* `util/` - data structures, string utilities, etc.


[POSIX]: https://pubs.opengroup.org/onlinepubs/9699919799/
[EditorConfig]: https://editorconfig.org/
