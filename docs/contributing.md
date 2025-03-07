Contributing
============

Please observe the following guidelines when creating new [issues]
or [merge requests] for dte.

Bugs
----

* Search for existing [issues] before opening new ones.
* Describe how the problem can be reproduced, if possible.
* Include any relevant error messages as Markdown [code blocks].
* Include relevant system information:
  * dte version (e.g. "dte 1.11.1")
  * Operating system (e.g. "Debian 12")
  * Terminal (e.g. "kitty 0.39.1")
  * Terminal multiplexer (if applicable, e.g. "tmux 3.5")
  * Virtualization software (if applicable, e.g. QEMU, WSL2, Docker, etc.)

Feature Requests
----------------

* Search for existing [feature request issues] before opening new ones.
* Also search [`TODO.md`]. Feel free to open issues for features already
  mentioned there, but only if there's something significant to discuss.
* Describe at least one real-world use case.

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
[feature request issues]: https://gitlab.com/craigbarnes/dte/-/issues/?sort=updated_desc&state=all&or%5Blabel_name%5D%5B%5D=Type%3A%3AFeature&or%5Blabel_name%5D%5B%5D=Type%3A%3AEnhancement&first_page_size=50
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
[code blocks]: https://docs.gitlab.com/user/markdown/#code-spans-and-blocks
[`TODO.md`]: TODO.md
[`pre-commit`]: ../tools/git-hooks/pre-commit
[`commit-msg`]: ../tools/git-hooks/commit-msg
