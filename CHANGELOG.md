Releases
========

v1.8.2 (latest release)
-----------------------

Released on 2019-04-23.

**Changes:**

* Fixed makefile to work with GNU Make 3.81 (which is still used
  by OS X and Ubuntu 14.04).

**Downloads:**

* [dte-1.8.2.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.2.tar.gz)
* [dte-1.8.2.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.2.tar.gz.sig)

v1.8.1
------

Released on 2019-04-22.

**Fixes:**

* Fixed parsing of escaped special characters in command arguments
  (which was causing Lua syntax highlighting to fail).
* Removed use of `rep` (repeat character) control sequence, due to
  problems caused by certain terminal emulators that claim to be
  "xterm" but don't support the full set of features in the xterm
  `terminfo(5)` entry (notably, the FreeBSD 12 console).

**Additions:**

* Show a confirmation prompt if Ctrl+q (quit) is pressed with unsaved
  changes, instead of a cryptic error message.

**Downloads:**

* [dte-1.8.1.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.1.tar.gz)
* [dte-1.8.1.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.1.tar.gz.sig)

v1.8
----

Released on 2019-04-18.

**Additions:**

* Added support for 24-bit RGB terminal colors.
* Added support for `strikethrough` terminal attribute.
* Added support for `alias` names containing multi-byte Unicode characters.
* Added `refresh` command (to force a full screen redraw).
* Added `dte -K` command-line option (for keycode debugging).
* Added support for reading a buffer from `stdin` at startup.
* Added support for binding Ctrl/Alt/Shift + F1-F12 on xterm-like terminals.
* Added `-s` flag to `bol` command, to allow moving to beginning of indented
  text, before moving to beginning of line (a.k.a "smart home").
* Added `-c` flag to all cursor movement commands, to allow selecting
  characters while moving.
* Added `-l` flag to `up`, `down`, `pgup` and `pgdown` commands, to
  allow selecting whole lines while moving.
* Added default bindings for various Shift+key combinations, for doing
  GUI-style text selections.
* Added key bindings to command mode for deleting/erasing whole words
  (Alt+Delete and Alt+Backspace).

**Improvements:**

* Optimized built-in filetype detection.
* Fixed cursor interaction with Unicode combining characters.
* Improved handling of Unicode whitespace and unprintable characters.
* Updated character class lookup tables to Unicode 11.
* Expanded documentation for `hi` and `compile` commands.
* Optimized code to reduce editor startup and input latency.

**Breaking changes:**

* Changed the `bind` command to be much more strict when parsing key
  strings and show an error message when invalid. The caret (`^`)
  modifier can now only be used alone (not in combination with other
  modifiers) and the `C-`, `M-` and `S-` modifiers *must* be
  uppercase.
* Removed support for chained key bindings (e.g. `bind '^X c' ...`).
  Commands that aren't bound to simple key combinations can just be
  accessed via command mode.
* Removed support for recognizing some Ctrl/Alt/Shift key combinations
  produced by the `rxvt` terminal emulator. The key codes produced by
  `rxvt` violate the [ECMA-48] specification. Users of such terminals
  are encouraged to configure the key codes to mimic `xterm` instead.

**Downloads:**

* [dte-1.8.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.tar.gz)
* [dte-1.8.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.8.tar.gz.sig)

v1.7
----

Released on 2018-05-08.

**Changes:**

* Added support for opening multiple files using glob patterns
  (e.g. `open -g *.[ch]`).
* Added support for binding more xterm extended key combinations
  (Ctrl/Meta/Shift + PageUp/PageDown/Home/End).
* Improved compiler error parsing for newer versions of GCC.
* Improved handling of underline/dim/italic terminal attributes
  (including support for the `ncv` terminfo capability).
* Many other small bug fixes and code improvements.

**Downloads:**

* [dte-1.7.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.7.tar.gz)
* [dte-1.7.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.7.tar.gz.sig)

v1.6
----

Released on 2017-12-20.

**Changes:**

* Added new, default `dark` color scheme.
* Added Ctrl+G key binding to exit command mode.
* Added Ctrl+H key binding for `erase` command.
* Added syntax highlighting for TeX and roff (man page) files.
* Improved syntax highlighting of Python numeric literals.
* Improved syntax highlighting for CSS files.
* Added `ft -b` command for detecting file types from file basenames.
* Converted user documentation to Markdown format.
* Created new [website] for online documentation.
* Added support for terminfo extended (or "user-defined") capabilities.
* Added built-in support for `st` and `rxvt` terminals.
* Fixed some built-in regex patterns to avoid non-portable features.
* Fixed compiler warnings on NetBSD.
* Removed tilde expansion of `~username` from command mode, in order to
  avoid using `getpwnam(3)` and thereby allow static linking with GNU
  libc on Linux.

**Downloads:**

* [dte-1.6.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.6.tar.gz)
* [dte-1.6.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.6.tar.gz.sig)

v1.5
----

Released on 2017-11-03.

**Changes:**

* Added syntax highlighting for Nginx config files.
* Added some POSIX and C11 features to the C syntax highlighter.
* Added new command-line flags for listing (`-B`) and dumping
  (`-b`) built-in rc files.
* Moved some of the documentation from the `dte(1)` man page to a
  separate `dterc(5)` page.
* Fixed a terminal input bug triggered by redirecting `stdin`.
* Fixed some memory and file descriptor leaks.
* Fixed a few portability issues.

**Downloads:**

* [dte-1.5.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.5.tar.gz)
* [dte-1.5.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.5.tar.gz.sig)

v1.4
----

Released on 2017-10-16.

**Changes:**

* Changed the build system to compile all default configs and syntax
  highlighting files into the `dte` binary instead of installing and
  loading them from disk. The `$PKGDATADIR` variable is now removed.
* Added syntax highlighting for the Vala and D languages.
* Added the ability to bind additional, xterm-style key combinations
  (e.g. `bind C-M-S-left ...`) when `$TERM` is `tmux` (previously
  only available for `xterm` and `screen`).
* Added an option to compile without linking to the curses/terminfo
  library (`make TERMINFO_DISABLE=1`), to make it easier to create
  portable, statically-linked builds.

**Downloads:**

* [dte-1.4.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.4.tar.gz)
* [dte-1.4.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.4.tar.gz.sig)

v1.3
----

Released on 2017-08-27.

**Changes:**

* Added support for binding Ctrl+Alt+Shift+arrows in xterm/screen/tmux.
* Added support for binding Ctrl+delete/Shift+delete in xterm/screen/tmux.
* Added the ability to override the default user config directory via
  the `DTE_HOME` environment variable.
* Added syntax highlighting for the Markdown, Meson and Ruby languages.
* Improved syntax highlighting for the C, awk and HTML languages.
* Fixed a bug with the `close -wq` command when using split frames
  (`wsplit`).
* Fixed a segfault bug in `git-open` mode when not inside a git repo.
* Fixed a few cases of undefined behaviour and potential buffer overflow
  inherited from [dex].
* Fixed all compiler warnings when building on OpenBSD 6.
* Fixed and clarified various details in the man pages.
* Renamed `inc-home` and `inc-end` commands to `bolsf` and `eolsf`,
  for consistency with other similar commands.
* Renamed `dte-syntax.7` man page to `dte-syntax.5` (users with an
  existing installation may want to manually delete `dte-syntax.7`).

**Downloads:**

* [dte-1.3.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.3.tar.gz)
* [dte-1.3.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.3.tar.gz.sig)

v1.2
----

Released on 2017-07-30.

**Changes:**

* Unicode 10 rendering support.
* Various build system fixes.
* Coding style fixes.

**Downloads:**

* [dte-1.2.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.2.tar.gz)
* [dte-1.2.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.2.tar.gz.sig)

v1.1
----

Released on 2017-07-29.

**Changes:**

* Renamed project from "dex" to "dte".
* Changed default key bindings to be more like most GUI applications.
* Added `-n` flag to `delete-eol` command, to enable deleting newlines
  if the cursor is at the of the line.
* Added `-p` flag to `save` command, to open a prompt if the current
  buffer has no existing filename.
* Added `inc-end` and `inc-home` commands that move the cursor
  incrementally to the end/beginning of the line/screen/file.
* Added a command-line option to jump to a specific line number after
  opening a file.
* Added syntax highlighting for `ini`, `robots.txt` and `Dockerfile`
  languages.
* Fixed a compilation error on OpenBSD.
* Replaced quirky command-line option parser with POSIX `getopt(3)`.

**Downloads:**

* [dte-1.1.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.1.tar.gz)
* [dte-1.1.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.1.tar.gz.sig)

v1.0
----

Released on 2015-04-28.

This is identical to the `v1.0` [release][dex v1.0] of [dex][] (the editor
from which this project was forked).

**Downloads:**

* [dte-1.0.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.0.tar.gz)
* [dte-1.0.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.0.tar.gz.sig)

Public Key
==========

A detached PGP signature file is provided for each release. The public
key for verifying these signatures is available to download at
<https://craigbarnes.gitlab.io/pubkey/0330BEB4.asc>
or from the keyserver at `hkps://hkps.pool.sks-keyservers.net`.

Checksums
=========

A list of SHA256 checksums for all release tarballs and signatures is
available at <https://craigbarnes.gitlab.io/dist/dte/dte-sha256sums.txt>.


[website]: https://craigbarnes.gitlab.io/dte/
[dex]: https://github.com/tihirvon/dex
[dex v1.0]: https://github.com/tihirvon/dex/releases/tag/v1.0
[ECMA-48]: https://www.ecma-international.org/publications/standards/Ecma-048.htm
