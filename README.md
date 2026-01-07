dte
===

A small and easy to use terminal text editor.

[![screenshot]][screenshot]

Features
--------

* Multiple buffers/tabs
* Unlimited [undo]/[redo]
* Regex [search] and [replace]
* Syntax highlighting
* Customizable [color schemes]
* Customizable [key bindings]
* [Command language] with [command-line] auto-completion
* [Macro] recording
* Unicode 17 compatible text rendering
* Support for multiple encodings (using [iconv])
* Jump to definition (using [ctags])
* Jump to [compiler error]
* [EditorConfig] support
* Extensible via [external commands] and [stdio]
* Fast startup (~10ms)
* Minimal run-time dependencies (just libc on most systems)
* Minimal build-time dependencies ([GNU Make] and a C99 compiler)
* Portable to any [POSIX] 2008 operating system
* Modern terminal support:
  * Dynamic feature queries (no [terminfo] database or curses library needed)
  * 24-bit RGB colors
  * [OSC 52] clipboard [copy][] (works over SSH)
  * [Kitty keyboard protocol][] (more key combos available for binding)
  * xterm's [`modifyOtherKeys`] keyboard protocol
  * ["synchronized updates"][] (helps eliminate screen tearing)

Installing
----------

`dte` can be installed via package manager on the following platforms:

| OS                        | Install command               |
|---------------------------|-------------------------------|
| [Debian]                  | `apt-get install dte`         |
| [Ubuntu]                  | `apt-get install dte`         |
| Arch Linux ([AUR])        | `yay -S dte`                  |
| [Void Linux]              | `xbps-install -S dte`         |
| [FreeBSD]                 | `pkg install dte`             |
| DragonFly BSD ([DPorts])  | `pkg install dte`             |
| [OpenBSD]                 | `pkg_add dte`                 |
| NetBSD ([pkgsrc])         | `pkg_add dte`                 |
| Android ([Termux])        | `pkg install dte`             |

Building
--------

To build from source, first ensure the following dependencies are
installed:

* [GCC] 4.8+ or [Clang]
* [GNU Make] 4.0+
* [iconv] library (usually provided by libc on Linux/FreeBSD)

...then download and unpack the latest [release] tarball:

    curl -LO https://craigbarnes.gitlab.io/dist/dte/dte-1.11.1.tar.gz
    tar -xzf dte-1.11.1.tar.gz
    cd dte-1.11.1

...and compile and install:

    make && sudo make install

The `make install` command installs the `dte` binary, man pages and
shell completion scripts for bash/zsh/fish. If you're installing to
a desktop machine and would also like a [desktop entry] launcher,
[`make install-full`] can be used instead.

If you're using macOS, it may be necessary to install a more recent version
of GNU Make (e.g. with [`brew`]) and then use `gmake` in place of `make`.
For example:

    brew install make
    gmake && sudo gmake install

Requirements
------------

In addition to the build dependencies mentioned above, the following are
also needed for a correctly/fully functioning editor:

* [POSIX] 2008 conforming operating system
* [ECMA-48] compatible terminal (supporting at least the [`CUP`] and [`EL`]
  control functions)
* [UTF-8][] [locale]

Documentation
-------------

After installing, you can access the documentation in man page format:

* [`man dte`]
* [`man dterc`]
* [`man dte-syntax`]

See also:

* [Contributor guidelines]
* [Packaging information]

Contact
-------

Questions and patches may be sent by email, to the address printed by:

    echo ofmisnmfbqgzdfchcbamilkoca | tr zka-hm-t @.m-ta-h

...although GitLab [issue reports] and [merge requests] are preferred,
when possible.

For general discussion, we also have a [`#dte` channel] on the [Libera.Chat]
IRC network.

Donations
---------

Donations can be made via [Liberapay]. All support is very much
appreciated and allows me to spend more time working on dte.

License
-------

Copyright © 2013-2026 Craig Barnes.\
Copyright © 2010-2015 Timo Hirvonen.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU [General Public License version 2], as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License version 2 for more details.


[`man dte`]: https://craigbarnes.gitlab.io/dte/dte.html
[`man dterc`]: https://craigbarnes.gitlab.io/dte/dterc.html
[`man dte-syntax`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html
[Command language]: https://craigbarnes.gitlab.io/dte/dterc.html
[command-line]: https://craigbarnes.gitlab.io/dte/dte.html#command-mode
[Macro]: https://craigbarnes.gitlab.io/dte/dterc.html#macro
[color schemes]: https://craigbarnes.gitlab.io/dte/dterc.html#hi
[compiler error]: https://craigbarnes.gitlab.io/dte/dterc.html#compile
[copy]: https://craigbarnes.gitlab.io/dte/dterc.html#copy:~:text=Copy%20to%20system%20clipboard
[external commands]: https://craigbarnes.gitlab.io/dte/dterc.html#external-commands
[key bindings]: https://craigbarnes.gitlab.io/dte/dterc.html#bind
[redo]: https://craigbarnes.gitlab.io/dte/dterc.html#redo
[replace]: https://craigbarnes.gitlab.io/dte/dterc.html#replace
[search]: https://craigbarnes.gitlab.io/dte/dterc.html#search
[undo]: https://craigbarnes.gitlab.io/dte/dterc.html#undo

[screenshot]: https://craigbarnes.gitlab.io/dte/screenshot.png
[iconv]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/iconv.h.html
[ctags]: https://ctags.io/
[stdio]: https://man7.org/linux/man-pages/man3/stdin.3.html#DESCRIPTION
[EditorConfig]: https://editorconfig.org/
[GNU Make]: https://www.gnu.org/software/make/
[POSIX]: https://pubs.opengroup.org/onlinepubs/9699919799/
[terminfo]: https://man7.org/linux/man-pages/man5/terminfo.5.html
[OSC 52]: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-Operating-System-Commands:~:text=5%202%20%20%E2%87%92%C2%A0%20Manipulate%20Selection%20Data
[Kitty keyboard protocol]: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
[`modifyOtherKeys`]: https://invisible-island.net/xterm/manpage/xterm.html#VT100-Widget-Resources:modifyOtherKeys
["synchronized updates"]: https://gitlab.freedesktop.org/terminal-wg/specifications/-/merge_requests/2
[UTF-8]: https://datatracker.ietf.org/doc/html/rfc3629
[locale]: https://man7.org/linux/man-pages/man7/locale.7.html
[ECMA-48]: https://ecma-international.org/publications-and-standards/standards/ecma-48/
[`CUP`]: https://vt100.net/docs/vt510-rm/CUP.html
[`EL`]: https://vt100.net/docs/vt510-rm/EL.html

[Debian]: https://packages.debian.org/source/dte
[Ubuntu]: https://launchpad.net/ubuntu/+source/dte
[AUR]: https://aur.archlinux.org/packages/dte/
[Void Linux]: https://github.com/void-linux/void-packages/tree/master/srcpkgs/dte
[FreeBSD]: https://cgit.freebsd.org/ports/tree/editors/dte
[DPorts]: https://github.com/DragonFlyBSD/DPorts/tree/master/editors/dte
[OpenBSD]: https://cvsweb.openbsd.org/cgi-bin/cvsweb/ports/editors/dte/
[pkgsrc]: https://cdn.netbsd.org/pub/pkgsrc/current/pkgsrc/editors/dte/index.html
[Termux]: https://github.com/termux/termux-packages/tree/master/packages/dte

[GCC]: https://gcc.gnu.org/
[Clang]: https://clang.llvm.org/
[release]: https://craigbarnes.gitlab.io/dte/releases.html
[desktop entry]: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html
[`make install-full`]: https://gitlab.com/craigbarnes/dte/-/blob/master/docs/packaging.md#installation-targets
[`brew`]: https://brew.sh/
[Contributor guidelines]: https://gitlab.com/craigbarnes/dte/-/blob/master/docs/contributing.md
[Packaging information]: https://gitlab.com/craigbarnes/dte/blob/master/docs/packaging.md
[issue reports]: https://gitlab.com/craigbarnes/dte/-/issues
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
[`#dte` channel]: https://web.libera.chat/?channels=#dte
[Libera.Chat]: https://libera.chat/
[Liberapay]: https://liberapay.com/craigbarnes/donate
[General Public License version 2]: https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
