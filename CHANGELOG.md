Releases
========

All releases and notable changes will be documented here, using a format
similar to the one described by [Keep a Changelog].

v1.11.1 (latest release)
------------------------

Released on 2023-03-01.

**Additions:**

* Added a [`show setenv`][`show`] sub-command.
* Added an `-m` flag to the [`undo`] command, to allow moving to the last
  change without undoing it.
* Added support for Super (`s-`) and Hyper (`H-`) modifiers in key bindings.
* Added a syntax highlighter for [Coccinelle] "semantic patch" (`*.cocci`)
  files.
* Added rules for handling CDATA sections to the XML syntax highlighter.
* Added auto-completion for command flags.

**Fixes:**

* Fixed a bug that was causing key bindings to not work properly when
  the Num Lock and/or Caps Lock modifiers were in effect in terminals
  supporting the [kitty keyboard protocol].
* Fixed the [`save`] command, to avoid breaking hard links when writing
  to existing files.
* Fixed an issue in the build system causing `make distcheck` to fail.

**Other changes:**

* Changed built-in filetype detection so that a `dot_` filename prefix is
  equivalent to a `.` prefix, or more specifically `dot_bashrc` is now
  treated the same way as `.bashrc`.
* Changed the [`default`] command in [`dte-syntax`] files to show an error
  if there are duplicate arguments.
* [Removed][commit 0fe5e5f224e832a] the built-in `xsel` alias.

**Downloads:**

* [dte-1.11.1.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.11.1.tar.gz)
* [dte-1.11.1.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.11.1.tar.gz.sig)

v1.11
-----

Released on 2023-01-21.

**Additions:**

* Added 15 new command flags:
  * [`bind -n`][`bind`]
  * [`bind -c`][`bind`] (custom key bindings in [`command`] mode)
  * [`bind -s`][`bind`] (custom key bindings in [`search`] mode)
  * [`bol -t`][`bol`]
  * [`bof -c`][`bof`]
  * [`bof -l`][`bof`]
  * [`eof -c`][`eof`]
  * [`eof -l`][`eof`]
  * [`clear -i`][`clear`]
  * [`copy -b`][`copy`] (copy to system clipboard)
  * [`copy -i`][`copy`]
  * [`copy -p`][`copy`]
  * [`paste -a`][`paste`]
  * [`paste -m`][`paste`]
  * [`new-line -a`][`new-line`]
* Added a new [`exec`] command, (this replaces `run`, `filter`, `pipe-from`,
  `pipe-to`, `eval`, `exec-open` and `exec-tag`, which are now just built-in
  [aliases][exec aliases] of [`exec`]).
* Added [`overwrite`] and [`optimize-true-color`] options.
* Added built-in [`$RFILE`], [`$FILEDIR`] and [`$COLNO`] variables.
* Added `hi`, `msg` and `set` arguments to the [`show`] command.
* Added a _number_ argument to the [`msg`] command.
* Added support for 3-digit `#rgb` colors to the [`hi`] command (in addition
  to the existing `#rrggbb` support).
* Added support for binding `F13`-`F20` keys.
* Added support for parsing alternate encodings of `F1`-`F4` keys
  (e.g. `CSI P`) sent by some terminals.
* Added support for the [Kitty keyboard protocol][] (which significantly
  increases the number of bindable key combos).
* Added syntax highlighters for JSON, Go Module (`go.mod`), G-code and
  `.gitignore` files.
* Added support for binary literals and hex float literals to the C syntax
  highlighter.
* Added support for escaped newlines in string literals to the C syntax
  highlighter.
* Added support for `<<-EOF` heredocs to the shell syntax highlighter.
* Added support for template literals to the JavaScript syntax highlighter.
* Added support for terminal "synchronized updates" (both the DCS-based
  variant and the private mode `2026` variant).
* Added documentation for the [`bookmark`] command.
* Added `$PATH`, `$PWD`, `$OLDPWD` and `$DTE_VERSION` to the [environment]
  section of the [`dte`] man page.
* Added extended support for [Contour] and [WezTerm] terminals.
* Added an [AppStream] metadata file, which is installed by default when
  running `make install` (except on macOS).

**Improvements:**

* Updated Unicode support to version 15.
* Allowed [`alias`] and [`errorfmt`] entries to be removed, by running
  the commands with only 1 argument.
* Improved command auto-completion for [`alias`], [`bind`], [`cd`],
  [`redo`], [`move-tab`] and [`quit`] commands.
* Improved documentation for [`alias`]. [`tag`], [`hi`], [`msg`] and
  [`wsplit`] commands.
* Changed [`quit -p`][`quit`] to display the number of modified/unsaved
  buffers in the dialog prompt.
* Excluded `.` and `..` from filename auto-completions.
* Allowed `-c <command>` options to be used multiple (up to 8) times.
* Allowed `+lineno,colno` command-line arguments (in addition to
  `+lineno`).
* Extended [`line`] command to accept a `lineno,colno` argument (in
  addition to `lineno`).
* Limited the size of `.editorconfig` files to 32MiB.
* Enabled "enhanced" regex features on macOS, by using the
  [`REG_ENHANCED`] flag when calling `regcomp(3)`.
* Improved support for the [`modifyOtherKeys`] keyboard mode (which
  increases the number of bindable key combos somewhat), by emitting
  the escape sequence to enable it at startup.
* Various improvements to built-in filetype detection.
* Various performance/efficiency improvements.

**Fixes:**

* Fixed a bug that caused searches started by [`search -r`][`search`] to
  be incorrectly recorded by `macro record`.
* Fixed a bug that caused "broken pipe" errors to occur if the terminal
  was resized during certain long-running commands (e.g. [`compile`]).
* Fixed several regular expressions in built-in configs that were using
  non-portable regex features (`\s`, `\b` and `\w`) and causing errors
  on some systems.
* [Fixed][commit c4af2b1a15c96e8] a portability issue that was causing
  execution of external commands to fail with "function not implemented"
  errors on some systems (notably Debian GNU/kFreeBSD).

**Breaking changes:**

* Changed the default Ctrl+v key binding to [`paste -a`][`paste`]
  (previously `paste -c`).
* Removed the `display-invisible` global option.
* Removed the `-b` flag from the `select` command.
* Renamed the built-in `coffeescript` filetype to `coffee`.
* Made the [`str`] command in [`dte-syntax`] files produce an error if
  used with single-byte arguments ([`char`] should be used instead).
* The `pipe-from` `-s` flag was effectively renamed to `-n`, as part of
  the [changes][commit d0c22068c340e79] made for the new [`exec`] command.

**Other changes:**

* Increased the minimum [GNU Make] version requirement to 4.0.
* Increased the minimum [GCC] version requirement to 4.8.

**Downloads:**

* [dte-1.11.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.11.tar.gz)
* [dte-1.11.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.11.tar.gz.sig)

v1.10
-----

Released on 2021-04-03.

**Additions:**

* Added 7 new commands:
  * [`blkdown`]
  * [`blkup`]
  * [`delete-line`]
  * `exec-open` (note: replaced by [`exec`] in v1.11)
  * `exec-tag` (note: replaced by [`exec`] in v1.11)
  * [`macro`]
  * [`match-bracket`]
* Added 12 new command flags:
  * [`include -q`][`include`]
  * [`hi -c`][`hi`]
  * `filter -l` (note: replaced by [`exec`] in v1.11)
  * `pipe-to -l` (note: replaced by [`exec`] in v1.11)
  * [`close -p`][`close`]
  * [`wclose -p`][`wclose`]
  * [`open -t`][`open`]
  * [`save -b`][`save`]
  * [`save -B`][`save`]
  * [`wsplit -t`][`wsplit`]
  * [`wsplit -g`][`wsplit`]
  * [`wsplit -n`][`wsplit`]
* Added 2 new global options:
  * [`select-cursor-char`]
  * [`utf8-bom`]
* Added an optional *exitcode* argument to the [`quit`] command.
* Added `color`, `command`, `env`, `errorfmt`, `ft`, `macro`, `option`,
  `search` and `wsplit` arguments to the [`show`] command.
* Added support for the `\e` escape sequence in [double-quoted] command
  arguments.
* Added syntax highlighting for Lisp and Scheme files.
* Added an Alt+Enter key binding to search mode, for performing
  plain-text searches.
* Added a Shift+Tab key binding to command mode, for iteratating
  auto-completions in reverse order.
* Added `%b`, `%N` and `%S` [statusline] format specifiers.
* Added a large confirmation dialog, shown when [`quit -p`][`quit`] is
  run with unsaved changes.
* Added the ability to exclude individual commands from command history
  (by prepending a space character when in command mode).

**Improvements:**

* Updated Unicode support to version 13.
* Bound Ctrl+c to [`copy -k`][`copy`] by default.
* Re-introduced built-in support for rxvt Ctrl/Alt/Shift key combinations.
* Fixed the handling of optional capture groups in [`errorfmt`] patterns.
* Improved the legibility of the default color scheme on a wider range
  of terminals.
* Changed the `filter` and `pipe-from` commands to set `$LINES`/`$COLUMNS`
  to the current window height/width, before running the specified program.
* Clarified which command flags in the [`dterc`] man page are mutually
  exclusive (by separating them with `|`).
* Fixed signal handling, to allow interrupting unresponsive/deadlocked
  child processes with Ctrl+c.
* Fixed command-line auto-completion to work properly when option flags
  are present.
* Improved the documentation for [`tag`], [`replace`], and [`errorfmt`].
* Various syntax highlighting improvements.
* Various terminal compatibility improvements.
* Various performance improvements.

**Breaking changes:**

* Removed support for linking to the system terminfo library. The
  terminfo database has only been used as a last resort source of
  information for several releases now. Most terminals that people
  are likely to be using already have built-in support in the editor,
  including several capabilities not available from terminfo. This is
  listed as a breaking change because it may break support for a few
  archaic hardware terminals (primarily those that aren't ECMA-48
  compatible or whose terminfo strings contain mandatory padding).
* Removed support for vertical tab bars (the `tab-bar` option was
  changed from an enum to a Boolean).

**Downloads:**

* [dte-1.10.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.10.tar.gz)
* [dte-1.10.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.10.tar.gz.sig)

v1.9.1
------

Released on 2019-09-29.

**Changes:**

* Fixed `make check` when running from a release tarball.

**Downloads:**

* [dte-1.9.1.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.9.1.tar.gz)
* [dte-1.9.1.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.9.1.tar.gz.sig)

v1.9
----

Released on 2019-09-21.

**Additions:**

* Added a new `pipe-to` command, to complement the existing `pipe-from`
  and `filter` commands.
* Added a new `show` command, which can be used to introspect the
  current values of aliases and bindings.
* Added a `-k` flag to the `copy` command, to allow keeping the current
  selection after copying.
* Added a man page entry for the (previously undocumented) `eval`
  command.
* Added new `$FILETYPE` and `$LINENO` special variables.
* Added a `display-invisible` global option, to allow visible rendering
  of otherwise invisible Unicode characters.
* Added an `-s` command-line flag, for validating custom syntax files.
* Added a compile-time `ICONV_DISABLE=1` option, which disables linking
  to the system iconv library (but makes the editor UTF-8 only).
* Added a Desktop Entry file, which can be installed using
  `make install-desktop-file`.

**Improvements:**

* Updated Unicode support to version 12.1.
* Modified the `errorfmt` command, to allow sub-match groups in the
  regexp pattern to be ignored.
* Various improvements to syntax highlighting and filetype detection.
* Various performance optimizations.

**Fixes:**

* Fixed a bug that would sometimes cause files to be saved as UTF-8,
  even if another encoding was specified.

**Downloads:**

* [dte-1.9.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.9.tar.gz)
* [dte-1.9.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.9.tar.gz.sig)

v1.8.2
------

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
* Removed tilde expansion of `~username/` from [`dterc`] commands, in
  order to avoid using `getpwnam(3)` and thereby allow static linking
  with [glibc] on Linux.

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
or from the keyserver at `hkps://keys.openpgp.org`.

Checksums
=========

A list of SHA256 checksums for all release tarballs and signatures is
available at <https://craigbarnes.gitlab.io/dist/dte/dte-sha256sums.txt>.

Portable Builds for Linux
=========================

Some pre-built, portable binaries are available for Linux. They're
statically-linked with [musl] libc and require nothing of the host
system except a somewhat recent kernel.

* [`dte-master-linux-x86_64.tar.gz`](https://craigbarnes.gitlab.io/dte/dte-master-linux-x86_64.tar.gz)
* [`dte-1.11.1-linux-x86_64.tar.gz`](https://craigbarnes.gitlab.io/dist/dte/dte-1.11.1-linux-x86_64.tar.gz)

*Note*: only `x86_64` builds are available for now. Feel free to open an
[issue] if you need builds for other architectures.


[website]: https://craigbarnes.gitlab.io/dte/
[Keep a Changelog]: https://keepachangelog.com/
[Contour]: https://github.com/contour-terminal/contour
[WezTerm]: https://wezfurlong.org/wezterm/
[Kitty keyboard protocol]: https://sw.kovidgoyal.net/kitty/keyboard-protocol/
[`modifyOtherKeys`]: https://invisible-island.net/xterm/manpage/xterm.html#VT100-Widget-Resources:modifyOtherKeys
[GNU Make]: https://www.gnu.org/software/make/
[GCC]: https://gcc.gnu.org/
[glibc]: https://sourceware.org/glibc/
[AppStream]: https://www.freedesktop.org/software/appstream/docs/
[Coccinelle]: https://coccinelle.gitlabpages.inria.fr/website/
[`REG_ENHANCED`]: https://www.unix.com/man-page/osx/7/re_format/
[commit d0c22068c340e79]: https://gitlab.com/craigbarnes/dte/-/commit/d0c22068c340e795f4e98e6d2bcea6a228f57403
[commit c4af2b1a15c96e8]: https://gitlab.com/craigbarnes/dte/-/commit/c4af2b1a15c96e820452c183e81e9bd415492778
[commit 0fe5e5f224e832a]: https://gitlab.com/craigbarnes/dte/-/commit/0fe5e5f224e832a382ce1fb7e6b4e0d6f0da8f55
[exec aliases]: https://gitlab.com/craigbarnes/dte/-/blob/14dd15b72383db784156b20d4ab7ac392552fe59/config/rc#L60-66
[dex]: https://github.com/tihirvon/dex
[dex v1.0]: https://github.com/tihirvon/dex/releases/tag/v1.0
[ECMA-48]: https://ecma-international.org/publications-and-standards/standards/ecma-48/
[musl]: https://www.musl-libc.org/
[issue]: https://gitlab.com/craigbarnes/dte/-/issues

[`dte`]: https://craigbarnes.gitlab.io/dte/dte.html
[environment]: https://craigbarnes.gitlab.io/dte/dte.html#environment

[`dte-syntax`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html
[`str`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html#str
[`char`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html#char
[`default`]: https://craigbarnes.gitlab.io/dte/dte-syntax.html#default

[`dterc`]: https://craigbarnes.gitlab.io/dte/dterc.html
[`alias`]: https://craigbarnes.gitlab.io/dte/dterc.html#alias
[`bind`]: https://craigbarnes.gitlab.io/dte/dterc.html#bind
[`blkdown`]: https://craigbarnes.gitlab.io/dte/dterc.html#blkdown
[`blkup`]: https://craigbarnes.gitlab.io/dte/dterc.html#blkup
[`bof`]: https://craigbarnes.gitlab.io/dte/dterc.html#bof
[`bol`]: https://craigbarnes.gitlab.io/dte/dterc.html#bol
[`bookmark`]: https://craigbarnes.gitlab.io/dte/dterc.html#bookmark
[`cd`]: https://craigbarnes.gitlab.io/dte/dterc.html#cd
[`clear`]: https://craigbarnes.gitlab.io/dte/dterc.html#clear
[`close`]: https://craigbarnes.gitlab.io/dte/dterc.html#close
[`command`]: https://craigbarnes.gitlab.io/dte/dterc.html#command
[`compile`]: https://craigbarnes.gitlab.io/dte/dterc.html#compile
[`copy`]: https://craigbarnes.gitlab.io/dte/dterc.html#copy
[`delete-line`]: https://craigbarnes.gitlab.io/dte/dterc.html#delete-line
[`eof`]: https://craigbarnes.gitlab.io/dte/dterc.html#eof
[`errorfmt`]: https://craigbarnes.gitlab.io/dte/dterc.html#errorfmt
[`exec`]: https://craigbarnes.gitlab.io/dte/dterc.html#exec
[`hi`]: https://craigbarnes.gitlab.io/dte/dterc.html#hi
[`include`]: https://craigbarnes.gitlab.io/dte/dterc.html#include
[`line`]: https://craigbarnes.gitlab.io/dte/dterc.html#line
[`macro`]: https://craigbarnes.gitlab.io/dte/dterc.html#macro
[`match-bracket`]: https://craigbarnes.gitlab.io/dte/dterc.html#match-bracket
[`move-tab`]: https://craigbarnes.gitlab.io/dte/dterc.html#move-tab
[`msg`]: https://craigbarnes.gitlab.io/dte/dterc.html#msg
[`new-line`]: https://craigbarnes.gitlab.io/dte/dterc.html#new-line
[`open`]: https://craigbarnes.gitlab.io/dte/dterc.html#open
[`paste`]: https://craigbarnes.gitlab.io/dte/dterc.html#paste
[`quit`]: https://craigbarnes.gitlab.io/dte/dterc.html#quit
[`redo`]: https://craigbarnes.gitlab.io/dte/dterc.html#redo
[`replace`]: https://craigbarnes.gitlab.io/dte/dterc.html#replace
[`save`]: https://craigbarnes.gitlab.io/dte/dterc.html#save
[`search`]: https://craigbarnes.gitlab.io/dte/dterc.html#search
[`show`]: https://craigbarnes.gitlab.io/dte/dterc.html#show
[`tag`]: https://craigbarnes.gitlab.io/dte/dterc.html#tag
[`undo`]: https://craigbarnes.gitlab.io/dte/dterc.html#undo
[`wclose`]: https://craigbarnes.gitlab.io/dte/dterc.html#wclose
[`wsplit`]: https://craigbarnes.gitlab.io/dte/dterc.html#wsplit

[double-quoted]: https://craigbarnes.gitlab.io/dte/dterc.html#double-quoted-strings
[`optimize-true-color`]: https://craigbarnes.gitlab.io/dte/dterc.html#optimize-true-color
[`overwrite`]: https://craigbarnes.gitlab.io/dte/dterc.html#overwrite
[`select-cursor-char`]: https://craigbarnes.gitlab.io/dte/dterc.html#select-cursor-char
[`utf8-bom`]: https://craigbarnes.gitlab.io/dte/dterc.html#utf8-bom
[statusline]: https://craigbarnes.gitlab.io/dte/dterc.html#statusline-left
[`$COLNO`]: https://craigbarnes.gitlab.io/dte/dterc.html#COLNO
[`$RFILE`]: https://craigbarnes.gitlab.io/dte/dterc.html#RFILE
[`$FILEDIR`]: https://craigbarnes.gitlab.io/dte/dterc.html#FILEDIR
