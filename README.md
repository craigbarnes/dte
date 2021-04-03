dte
===

A small and easy to use console text editor.

Features
--------

* Multiple buffers/tabs
* Unlimited undo/redo
* Regex search and replace
* Syntax highlighting
* Customizable color schemes
* Customizable key bindings
* Support for all xterm Ctrl/Alt/Shift key codes
* Command language with auto-completion
* Unicode 13 compatible text rendering
* Support for multiple encodings (using [iconv])
* Jump to definition (using [ctags])
* Jump to compiler error
* [EditorConfig] support

Screenshot
----------

![dte screenshot](https://craigbarnes.gitlab.io/dte/screenshot.png)

Installing
----------

`dte` can be installed via package manager on the following platforms:

| OS                        | Install command                            |
|---------------------------|--------------------------------------------|
| [Debian Testing]          | `apt-get install dte`                      |
| [Ubuntu]                  | `apt-get install dte`                      |
| Arch Linux ([AUR])        | `$AUR_HELPER -S dte`                       |
| [Void Linux]              | `xbps-install -S dte`                      |
| Slackware ([SlackBuilds]) | See: [SlackBuild Usage HOWTO]              |
| [FreeBSD]                 | `pkg install dte`                          |
| DragonFly BSD ([DPorts])  | `pkg install dte`                          |
| [OpenBSD]                 | `pkg_add dte`                              |
| NetBSD ([pkgsrc])         | `pkg_add dte`                              |
| OS X ([Homebrew])         | `brew tap yumitsu/dte && brew install dte` |
| Android ([Termux])        | `pkg install dte`                          |

Building
--------

To build from source, first ensure the following dependencies are
installed:

* [GCC] 4.6+ or [Clang]
* [GNU Make] 3.81+
* [iconv] library (usually provided by libc on Linux/FreeBSD)

...then download and unpack the latest release tarball:

    curl -LO https://craigbarnes.gitlab.io/dist/dte/dte-1.10.tar.gz
    tar -xzf dte-1.10.tar.gz
    cd dte-1.10

...and compile and install:

    make && sudo make install

Documentation
-------------

After installing, you can access the documentation in man page format
via `man 1 dte`, `man 5 dterc` and `man 5 dte-syntax`.

Online documentation is also available at <https://craigbarnes.gitlab.io/dte/>.

Packaging
---------

See [`docs/packaging.md`](https://gitlab.com/craigbarnes/dte/blob/master/docs/packaging.md).

License
-------

Copyright (C) 2017-2021 Craig Barnes.  
Copyright (C) 2010-2015 Timo Hirvonen.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU [General Public License version 2], as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License version 2 for more details.


[ctags]: https://ctags.io/
[EditorConfig]: https://editorconfig.org/
[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/
[`GNUmakefile`]: https://gitlab.com/craigbarnes/dte/blob/master/GNUmakefile
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[General Public License version 2]: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
[Debian Testing]: https://packages.debian.org/testing/dte
[Ubuntu]: https://launchpad.net/ubuntu/+source/dte
[AUR]: https://aur.archlinux.org/packages/dte/
[Void Linux]: https://github.com/void-linux/void-packages/tree/master/srcpkgs/dte
[SlackBuilds]: https://slackbuilds.org/repository/14.2/development/dte/
[SlackBuild Usage HOWTO]: https://slackbuilds.org/howto/
[FreeBSD]: https://svnweb.freebsd.org/ports/head/editors/dte/
[DPorts]: https://gitweb.dragonflybsd.org/dports.git/tree/HEAD:/editors/dte
[OpenBSD]: https://cvsweb.openbsd.org/cgi-bin/cvsweb/ports/editors/dte/
[pkgsrc]: https://pkgsrc.se/editors/dte
[Homebrew]: https://github.com/yumitsu/homebrew-dte
[Termux]: https://github.com/termux/termux-packages/tree/master/packages/dte
