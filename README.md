dte
===

dte is a small and easy to use console text editor.

Features
--------

* Multiple buffers/tabs
* Unlimited undo/redo
* Regex search and replace
* Syntax highlighting
* Customizable color schemes
* Customizable key bindings
* Command language with auto-completion
* Unicode 11 compatible text rendering
* Support for multiple encodings (using [iconv])
* Jump to definition (using [ctags])
* Jump to compiler error

Screenshot
----------

![dte screenshot](https://craigbarnes.gitlab.io/dte/screenshot.png)

Requirements
------------

* [GCC] 4.6+ or [Clang]
* [GNU Make] 3.81+
* [terminfo] library (may be provided by [ncurses], depending on OS)
* [iconv] library (may be included in libc, depending on OS)
* [POSIX]-compatible [`sh`] and [`awk`]

Installation
------------

To build `dte` from source, first install the requirements listed above,
then use the following commands:

    curl -LO https://craigbarnes.gitlab.io/dist/dte/dte-1.7.tar.gz
    tar -xzf dte-1.7.tar.gz
    cd dte-1.7
    make -j8 && sudo make install

Documentation
-------------

After installing, you can access the documentation in man page format
via `man 1 dte`, `man 5 dterc` and `man 5 dte-syntax`.

Online documentation is also available at <https://craigbarnes.gitlab.io/dte/>.

Testing
-------

`dte` is tested on the following platforms:

**[GitLab CI]**:

* Alpine Linux
* CentOS
* Debian
* Ubuntu
* Void Linux (musl)

**[Travis CI]**:

* Mac OS X
* Ubuntu

**Manual testing**:

* FreeBSD
* NetBSD
* OpenBSD

Other [POSIX] 2008 compatible platforms should also work, but may
require build system fixes.

Packaging
---------

The following optional build variables may be useful when packaging
`dte`:

* `prefix`: Top-level installation prefix (defaults to `/usr/local`).
* `bindir`: Installation prefix for program binary (defaults to
  `$prefix/bin`).
* `mandir`: Installation prefix for manual pages (defaults to
  `$prefix/share/man`).
* `DESTDIR`: Standard variable used for [staged installs].
* `V=1`: Enable verbose build output.
* `TERMINFO_DISABLE=1`: Use built-in terminal support, instead of
  linking to the system [terminfo]/curses library. This makes it much
  easier to build a portable, statically linked binary. The built-in
  terminal support currently works with `tmux`, `screen`, `st`, `xterm`
  (and many other `xterm`-compatible terminals) and falls back to
  [ECMA-48] mode for other terminals.

**Example usage**:

    make V=1
    make install V=1 prefix=/usr DESTDIR=PKG

License
-------

Copyright (C) 2017-2019 Craig Barnes.  
Copyright (C) 2010-2015 Timo Hirvonen.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU [General Public License version 2], as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License version 2 for more details.


[ctags]: https://en.wikipedia.org/wiki/Ctags
[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/
[ncurses]: https://www.gnu.org/software/ncurses/
[terminfo]: https://en.wikipedia.org/wiki/Terminfo
[ECMA-48]: http://www.ecma-international.org/publications/standards/Ecma-048.htm "ANSI X3.64 / ECMA-48 / ISO/IEC 6429"
[`GNUmakefile`]: https://gitlab.com/craigbarnes/dte/blob/master/GNUmakefile
[syntax files]: https://gitlab.com/craigbarnes/dte/tree/master/config/syntax
[staged installs]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[POSIX]: http://pubs.opengroup.org/onlinepubs/9699919799/
[iconv]: http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[`sh`]:  http://pubs.opengroup.org/onlinepubs/9699919799/utilities/sh.html
[`awk`]: http://pubs.opengroup.org/onlinepubs/9699919799/utilities/awk.html
[GitLab CI]: https://gitlab.com/craigbarnes/dte/pipelines
[Travis CI]: https://travis-ci.org/craigbarnes/dte
[General Public License version 2]: https://www.gnu.org/licenses/gpl-2.0.html
