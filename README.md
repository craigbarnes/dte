dte
===

dte is a small and easy to use console text editor.

Features
--------

* Multiple buffers/tabs
* Unlimited undo/redo
* Search and replace
* Syntax highlighting
* Customizable color schemes
* Customizable key bindings
* Command language with auto-completion
* Jump to definition (using [ctags])
* Jump to compiler error

Screenshot
----------

![dte screenshot](https://craigbarnes.gitlab.io/dte/screenshot.png)

Requirements
------------

* Modern C compiler (tested with [GCC] and [Clang])
* [GNU Make] `>= 3.81`
* [ncurses]

Installation
------------

You need to specify all options for both `make` and `make install`.
Alternatively you can put your build options into a `Config.mk` file.

    make -j8 prefix="$HOME/.local"
    make install prefix="$HOME/.local"

The default [`prefix`] is `/usr/local` and [`DESTDIR`] works as usual. See
the top of [`GNUmakefile`] for more information.

Documentation
-------------

After installing, you can access the documentation in man page format
via `man dte` and `man dte-syntax`.

These pages are also available as [dte.1.html] and [dte-syntax.5.html].

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


[ctags]: https://en.wikipedia.org/wiki/Ctags
[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/
[ncurses]: https://www.gnu.org/software/ncurses/
[`GNUmakefile`]: https://github.com/craigbarnes/dte/blob/master/GNUmakefile
[dte.1.html]: https://craigbarnes.gitlab.io/dte/dte.1.html
[dte-syntax.5.html]: https://craigbarnes.gitlab.io/dte/dte-syntax.5.html
[`prefix`]: https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
[`DESTDIR`]: https://www.gnu.org/prep/standards/html_node/DESTDIR.html
[General Public License version 2]: https://www.gnu.org/licenses/gpl-2.0.html
