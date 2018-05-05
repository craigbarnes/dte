Issues
------

* Check for existing [bug reports] before opening a new one.
* Include relevant error messages and/or test cases (formatted as
  Markdown [code blocks]). Avoid linking to external paste services.
* Include relevant system information (as printed by `make vars`).
* Please do not use the issue tracker to make "feature requests".
  Pull requests are very welcome though.

Pull Requests
-------------

* Create a separate git branch for each pull request.
* Use `git rebase` to avoid merge commits and fix-up commits.

Commits
-------

* Run `make git-hooks` **before** creating any commits. This installs
  a git [`pre-commit`] hook that automatically builds and tests the code
  and a [`commit-msg`] hook that checks commit message formatting.

Coding Style
------------

* Above all else, be consistent with the existing code.


[bug reports]: https://github.com/craigbarnes/dte/issues
[code blocks]: https://help.github.com/articles/creating-and-highlighting-code-blocks/#fenced-code-blocks
[`pre-commit`]: https://github.com/craigbarnes/dte/blob/master/tools/git-hooks/pre-commit
[`commit-msg`]: https://github.com/craigbarnes/dte/blob/master/tools/git-hooks/commit-msg
