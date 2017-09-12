dte changelog
=============

v1.4 (unreleased)
-----------------

**Changes:**

* Added syntax highlighting for the [Vala] language.

Git diff: [v1.3...master](https://github.com/craigbarnes/dte/compare/v1.3...master)

v1.3 (latest release)
---------------------

Released on 2017-08-27.

**Changes:**

* Added support for binding Ctrl+Alt+Shift+arrows in xterm/screen/tmux.
* Added support for binding Ctrl+delete/Shift+delete in xterm/screen/tmux.
* Added the ability to override the default user config directory via
  the `DTE_EDITOR` enviornment variable.
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


[dex]: https://github.com/tihirvon/dex
[Vala]: https://en.wikipedia.org/wiki/Vala_(programming_language)
