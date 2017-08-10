dte Change Log
--------------

Version 1.3 (not yet released)
------------------------------

* Added the ability to override the default user config directory via
  the `DTE_EDITOR` enviornment variable.
* Added syntax highlighting for the Markdown, Meson and Ruby languages.
* Added syntax highlighting for HTML5 tags and all HTML5 named entities.
* Fixed a bug with the `close -wq` command when using split frames
  (`wsplit`).
* Fixed a segfault bug in `git-open` mode when not inside a git repo.
* Fix a few cases of undefined behaviour and potential buffer overflow
  inherited from [dex].
* Fixed all compiler warnings when building on OpenBSD 6.
* Added some descriptive comments to the default configs.
* Various small fixes and clarifications to the man pages.

Version 1.2 (latest release)
----------------------------

* Unicode 10 support
* Build system fixes

Full diff: <https://github.com/craigbarnes/dte/compare/v1.1...v1.2>  
Download: [dte-1.2.tar.gz](https://craigbarnes.gitlab.io/dte/dist/dte-1.2.tar.gz)

Version 1.1
-----------

Full diff: <https://github.com/craigbarnes/dte/compare/v1.0...v1.1>  
Download: [dte-1.1.tar.gz](https://craigbarnes.gitlab.io/dte/dist/dte-1.1.tar.gz)


[dex]: https://github.com/tihirvon/dex
