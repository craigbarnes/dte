dte documentation
=================

This directory contains the documentation for `dte`. The main man pages
are authored in Markdown format, but the [Pandoc]-generated roff files
(`dte*.[15]`) are also checked into the repository. This is done mostly
for user convenience, so that `make && sudo make install` can be used
without the need for a full Pandoc installation.

When the [`pre-commit`] hook has been installed (with `make git-hooks`),
git will abort any attempt to commit changes to Markdown files without
corresponding updates to the generated files.

The easiest way to simply view the man pages is by running e.g. `man dterc`
after installation or by browsing the [online documentation].


[Pandoc]: https://pandoc.org/
[`pre-commit`]: https://gitlab.com/craigbarnes/dte/-/blob/master/tools/git-hooks/pre-commit
[online documentation]: https://craigbarnes.gitlab.io/dte/
