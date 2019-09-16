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


[EditorConfig]: https://editorconfig.org/
