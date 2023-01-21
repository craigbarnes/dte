dte
===

A small and easy to use console text editor.

Features
--------

* Multiple buffers/tabs
* Unlimited [undo]/[redo]
* Regex [search] and [replace]
* Syntax highlighting
* Customizable [color schemes] (including 24-bit RGB)
* Customizable [key bindings]
* [Command language] with auto-completion
* Unicode 15 compatible text rendering
* Support for all xterm key combos (including [`modifyOtherKeys`])
* Support for [kitty's keyboard protocol]
* Support for multiple encodings (using [iconv])
* Jump to definition (using [ctags])
* Jump to [compiler error]
* [Copy] to system clipboard (using [OSC 52], which works over SSH)
* [EditorConfig] support
* Fast startup (~10ms)

Screenshot
----------

![dte screenshot](https://craigbarnes.gitlab.io/dte/screenshot.png)

Installing
----------

`dte` can be installed via package manager on the following platforms:

| OS                        | Install command                            |
|---------------------------|--------------------------------------------|
| [Debian]                  | `apt-get install dte`                      |
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

* [GCC] 4.8+ or [Clang]
* [GNU Make] 4.0+
* [iconv] library (usually provided by libc on Linux/FreeBSD)

...then download and unpack the latest release tarball:

    curl -LO https://craigbarnes.gitlab.io/dist/dte/dte-1.11.tar.gz
    tar -xzf dte-1.11.tar.gz
    cd dte-1.11

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

Contributing
------------

See:

* [`docs/contributing.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/docs/contributing.md)
* [`src/README.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/src/README.md)
* [`config/README.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/config/README.md)
* [`mk/README.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/mk/README.md)
* [`mk/feature-test/README.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/mk/feature-test/README.md)
* [`tools/README.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/tools/README.md)
* [`docs/releasing.md`](https://gitlab.com/craigbarnes/dte/-/blob/master/docs/releasing.md)

Contact
-------

Questions and patches may be sent by email to <craigbarnes@protonmail.com>,
although GitLab [issue reports] and [merge requests] are usually preferred,
when possible.

For general discussion, we also have a `#dte` channel on the [Libera.Chat]
IRC network.

Donations
---------

Donations can be made via [Liberapay]. All support is very much
appreciated and allows me to spend more time working on dte.

License
-------

Copyright (C) 2013-2023 Craig Barnes.  
Copyright (C) 2010-2015 Timo Hirvonen.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU [General Public License version 2], as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License version 2 for more details.


[undo]: https://craigbarnes.gitlab.io/dte/dterc.html#undo
[redo]: https://craigbarnes.gitlab.io/dte/dterc.html#redo
[search]: https://craigbarnes.gitlab.io/dte/dterc.html#search
[replace]: https://craigbarnes.gitlab.io/dte/dterc.html#replace
[color schemes]: https://craigbarnes.gitlab.io/dte/dterc.html#hi
[key bindings]: https://craigbarnes.gitlab.io/dte/dterc.html#bind
[Command language]: https://craigbarnes.gitlab.io/dte/dterc.html
[`modifyOtherKeys`]: https://invisible-island.net/xterm/manpage/xterm.html#VT100-Widget-Resources:modifyOtherKeys
[kitty's keyboard protocol]: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[ctags]: https://ctags.io/
[compiler error]: https://craigbarnes.gitlab.io/dte/dterc.html#compile
[Copy]: https://craigbarnes.gitlab.io/dte/dterc.html#copy
[OSC 52]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Operating-System-Commands
[EditorConfig]: https://editorconfig.org/
[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/
[General Public License version 2]: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
[Debian]: https://packages.debian.org/source/dte
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
[issue reports]: https://gitlab.com/craigbarnes/dte/-/issues
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
[Libera.Chat]: https://libera.chat/
[Liberapay]: https://liberapay.com/craigbarnes/donate
