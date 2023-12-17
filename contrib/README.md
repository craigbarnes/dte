dte Contributed Scripts
=======================

This directory contains various example scripts that are intended to be
used in conjunction with the [`exec`] and [`compile`] commands. Feel
free to send [merge requests] adding new scripts here. Using `sh` or `awk`
as the script interpreter is a plus (for portability reasons), although
it's not a strict requirement.

Usage Examples
--------------

Bind Alt+h to open the `*.h` file matching the current `*.c` file
(or vice versa):

```sh
bind M-h 'exec -s -o open $DTE_HOME/scripts/open-c-header.sh $FILE'
```

Create an [`alias`] that jumps to the locations of all unstaged
changes made to the git working tree:

```sh
errorfmt gitdiff '^([^:]+): [+-]([0-9]+).*' file line
alias git-changes 'compile -1s gitdiff $DTE_HOME/scripts/git-changes.sh'
```

Create an [`alias`] that jumps to the longest line in the current buffer:

```sh
alias longest-line 'unselect; exec -s -i buffer -o eval awk -f $DTE_HOME/scripts/longest-line.awk'
```

Bind Ctrl+o to run a customized `fzf` file picker and open the selected files
(see the comments in the script for additional details):

```sh
bind C-o 'exec -o eval $DTE_HOME/scripts/fzf.sh'
```

Create an [`alias`] that jumps to the tag under the cursor, or opens an
`fzf` menu if there are multiple matching tags:

```sh
alias xtag 'exec -s -o eval -e errmsg $DTE_HOME/scripts/xtag.sh $WORD'
```

*Note:* the above examples assume you've copied the scripts in question
to `$DTE_HOME/scripts/` (i.e. `~/.dte/scripts/`).


[`alias`]: https://craigbarnes.gitlab.io/dte/dterc.html#alias
[`compile`]: https://craigbarnes.gitlab.io/dte/dterc.html#compile
[`exec`]: https://craigbarnes.gitlab.io/dte/dterc.html#exec
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
