dte Change Log
--------------

v1.3 (unreleased)
-----------------

* Added support for binding Ctrl+Alt+Shift+arrows in xterm/screen/tmux.
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

v1.2 (latest release)
---------------------

Released on 2017-07-30.

* Unicode 10 rendering support.
* Various build system fixes.

Git diff: [v1.1...v1.2](https://github.com/craigbarnes/dte/compare/v1.1...v1.2)  
Download: [dte-1.2.tar.gz](https://craigbarnes.gitlab.io/dte/dist/dte-1.2.tar.gz)

v1.1
----

Released on 2017-07-29.

Git diff: [v1.0...v1.1](https://github.com/craigbarnes/dte/compare/v1.0...v1.1)  
Download: [dte-1.1.tar.gz](https://craigbarnes.gitlab.io/dte/dist/dte-1.1.tar.gz)


[dex]: https://github.com/tihirvon/dex
