dte
===

Dexterous Text Editor

Introduction
------------

dte is a small and easy to use text editor. Colors and bindings can be
fully customized to your liking.

It has some features useful to programmers, like ctags support and it
can parse compiler errors, but it does not aim to become an IDE.

Installation
------------

The only dependencies are libc and ncurses.

To compile this program you need [GNU make] and a modern C compiler
(tested with [GCC] and [Clang]).

You need to specify all options for both `make` and `make install`.
Alternatively you can put your build options into a `Config.mk` file.

    make prefix="$HOME/.local"
    make install prefix="$HOME/.local"

The default `prefix` is `/usr/local` and [`DESTDIR`] works as usual. See
the top of [`GNUmakefile`] for more information.

License
-------

Copyright (C) 2017 Craig Barnes.  
Copyright (C) 2010-2015 Timo Hirvonen.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU [General Public License version 2], as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License version 2 for more details.


[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/
[`GNUmakefile`]: https://github.com/dte-editor/dte/blob/master/GNUmakefile
[`DESTDIR`]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[General Public License version 2]: https://www.gnu.org/licenses/gpl-2.0.html
