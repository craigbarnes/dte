dte tools
=========

This directory contains several small scripts used for development
purposes and is *not* included in [releases]:

* `mk/*.mk`: Various `make` utility targets (see `make help` output)
* `coccinelle/*.cocci`: [Coccinelle][] [semantic patches] used by the
  [`make check-coccinelle`] target (to identify some codebase-specific
  pitfalls or inefficiencies)
* `mock-headers/*.h`: Mock header files used by `make check-clang-tidy`,
  in place of some generated (`build/gen/*.h`) headers
* `git-hooks/*`: See [`docs/contributing.md`] and [`githooks(5)`]
* `*.{sh,awk}`: See comments at the top of each script


[releases]: https://craigbarnes.gitlab.io/dte/releases.html
[Coccinelle]: https://coccinelle.gitlabpages.inria.fr/website/
[semantic patches]: https://coccinelle.gitlabpages.inria.fr/website/sp.html
[`make check-coccinelle`]: mk/dev.mk
[`docs/contributing.md`]: docs/contributing.md#merge-requests:~:text=make%20git-hooks
[`githooks(5)`]: https://man7.org/linux/man-pages/man5/githooks.5.html#HOOKS
