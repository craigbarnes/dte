Issues
------

* Check for existing [bug reports] before opening a new one.
* Include any relevant error messages as Markdown [code blocks].
* Avoid linking to external paste services.
* Include relevant system information (as printed by `make vars`).

Pull Requests
-------------

* Create a separate git branch for each pull request.
* Use `git rebase` to avoid merge commits and fix-up commits.

Commits
-------

* Run `make git-hooks` **before** creating any commits. This installs
  a git [`pre-commit`] hook that automatically builds and tests each
  commit and a [`commit-msg`] hook that checks commit message
  formatting.

Coding Style
------------

* Above all else, be consistent with the existing code.


[bug reports]: https://gitlab.com/craigbarnes/dte/-/issues
[code blocks]: https://docs.gitlab.com/ee/user/markdown.html#code-and-syntax-highlighting
[`pre-commit`]: https://gitlab.com/craigbarnes/dte/blob/master/tools/git-hooks/pre-commit
[`commit-msg`]: https://gitlab.com/craigbarnes/dte/blob/master/tools/git-hooks/commit-msg
