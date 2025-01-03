Contributing
============

Please observe the following guidelines when creating new [issues]
or [merge requests] for dte.

Issues
------

* Search for existing [issues] before opening new ones.
* Include any relevant error messages as Markdown [code blocks].
* Include relevant system information (as printed by `make vars`).
* If suggesting new features:
  * Mention at least one real-world use case in the opening description.
  * Search [`TODO.md`] first. Feel free to open issues for features
    already mentioned there, but only if there's something substantive
    to discuss.

Merge Requests
--------------

* Create a separate git branch for each merge request.
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

* [`src/README.md`](../src/README.md)
* [`config/README.md`](../config/README.md)
* [`mk/README.md`](../mk/README.md)
* [`mk/feature-test/README.md`](../mk/feature-test/README.md)
* [`test/README.md`](../test/README.md)
* [`tools/README.md`](../tools/README.md)
* [`docs/README.md`](README.md)
* [`docs/TODO.md`](TODO.md)
* [`docs/packaging.md`](packaging.md)
* [`docs/releasing.md`](releasing.md)
* `make help`
* [GitLab CI pipelines](https://gitlab.com/craigbarnes/dte/-/pipelines)
* [Test coverage](https://craigbarnes.gitlab.io/dte/coverage/)


[issues]: https://gitlab.com/craigbarnes/dte/-/issues
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
[code blocks]: https://docs.gitlab.com/ee/user/markdown.html#code-spans-and-blocks
[`TODO.md`]: TODO.md
[`pre-commit`]: ../tools/git-hooks/pre-commit
[`commit-msg`]: ../tools/git-hooks/commit-msg
