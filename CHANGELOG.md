dte changelog
=============

v1.3 (unreleased)
-----------------

**Changes:**

* Added support for binding Ctrl+Alt+Shift+arrows in xterm/screen/tmux.
* Added support for binding Ctrl+delete/Shift+delete in xterm/screen/tmux.
* Added the ability to override the default user config directory via
  the `DTE_EDITOR` enviornment variable.
* Added syntax highlighting for the Markdown, Meson and Ruby languages.
* Added syntax highlighting for HTML5 tags and all HTML5 named entities.
* Added support for highlighting capitalized ClassNames to the C syntax
  highlighter (can be used via `hi class-name green`).
* Fixed a bug with the `close -wq` command when using split frames
  (`wsplit`).
* Fixed a segfault bug in `git-open` mode when not inside a git repo.
* Fixed a few cases of undefined behaviour and potential buffer overflow
  inherited from [dex].
* Fixed all compiler warnings when building on OpenBSD 6.
* Added some descriptive comments to the default config files.
* Fixed and clarified a few small details in the man pages.
* Renamed `inc-home` and `inc-end` commands to `bolsf` and `eolsf`,
  for consistency with other similar commands.
* Renamed `dte-syntax.7` man page to `dte-syntax.5`. Users with an
  installation of `dte` from before this change may want to manually
  delete the old section 7 man page, so that `man dte-syntax` always
  uses the new section 5 page.

Git diff: [v1.2...master](https://github.com/craigbarnes/dte/compare/v1.2...master)

v1.2 (latest release)
---------------------

Released on 2017-07-30.

**Changes:**

* Unicode 10 rendering support.
* Various build system fixes.

Git diff: [v1.1...v1.2](https://github.com/craigbarnes/dte/compare/v1.1...v1.2)

**Downloads:**

* [dte-1.2.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.2.tar.gz)
* [dte-1.2.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.2.tar.gz.sig)

v1.1
----

Released on 2017-07-29.

**Changes:**

Git diff: [v1.0...v1.1](https://github.com/craigbarnes/dte/compare/v1.0...v1.1)

**Downloads:**

* [dte-1.1.tar.gz](https://craigbarnes.gitlab.io/dist/dte/dte-1.1.tar.gz)
* [dte-1.1.tar.gz.sig](https://craigbarnes.gitlab.io/dist/dte/dte-1.1.tar.gz.sig)


[dex]: https://github.com/tihirvon/dex
