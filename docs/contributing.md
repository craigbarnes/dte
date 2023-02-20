Contributing
============

Please observe the following guidelines when creating new [issues]
or [pull requests] for dte.

Issues
------

* Check for existing [issues] before opening a new one.
* Include any relevant error messages as Markdown [code blocks].
* Avoid linking to external paste services.
* Include relevant system information (as printed by `make vars`).
* If suggesting new features, mention *at least* one real-world
  use case in the opening description.

Pull Requests
-------------

* Create a separate git branch for each pull request.
* Use `git rebase` to avoid merge commits and fix-up commits.
* Run `make git-hooks` **before** creating any commits. This installs
  a git [`pre-commit`] hook that automatically builds and tests each
  commit and a [`commit-msg`] hook that checks commit message
  formatting.

Coding Style
------------

* Above all else, be consistent with the existing code.

See Also
--------

* [`src/README.md`]
* [`config/README.md`]
* [`mk/README.md`]
* [`mk/feature-test/README.md`]
* [`tools/README.md`]
* [`docs/README.md`]
* [`docs/packaging.md`]
* [`docs/releasing.md`]


[issues]: https://gitlab.com/craigbarnes/dte/-/issues
[pull requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
[code blocks]: https://docs.gitlab.com/ee/user/markdown.html#code-spans-and-blocks
[`pre-commit`]: https://gitlab.com/craigbarnes/dte/blob/master/tools/git-hooks/pre-commit
[`commit-msg`]: https://gitlab.com/craigbarnes/dte/blob/master/tools/git-hooks/commit-msg
[`src/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/src/README.md
[`config/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/config/README.md
[`mk/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/mk/README.md
[`mk/feature-test/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/mk/feature-test/README.md
[`tools/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/tools/README.md
[`docs/README.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/docs/README.md
[`docs/packaging.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/docs/packaging.md
[`docs/releasing.md`]: https://gitlab.com/craigbarnes/dte/-/blob/master/docs/releasing.md
