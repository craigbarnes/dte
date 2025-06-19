dte Contributed Scripts
=======================

This directory contains various example scripts that are intended to be
used in conjunction with the [`exec`] and [`compile`] commands.

## Switch between `*.c` and `*.h` files

Bind Alt+h to open the `*.h` file matching the current `*.c` file
(or vice versa):

```sh
bind M-h 'exec -s -o open $DTE_HOME/scripts/open-c-header.sh $FILE'
```

## Navigate git index

Create an [`alias`] that jumps to the locations of all unstaged
changes made to the git working tree:

```sh
errorfmt gitdiff '^([^:]+): [+-]([0-9]+).*' file line
alias git-changes 'compile -1s gitdiff $DTE_HOME/scripts/git-changes.sh'
```

## Find longest line

Create an [`alias`] that jumps to the longest line in the current buffer:

```sh
alias longest-line 'unselect; exec -s -i buffer -o eval awk -f $DTE_HOME/scripts/longest-line.awk'
```

## Extended fzf file picker

Bind Ctrl+o to run a customized `fzf` file picker and open the selected files
(see the comments in the script for additional details):

```sh
bind C-o 'exec -o eval $DTE_HOME/scripts/fzf.sh'
```

## Extended fzf ctag picker

Create an [`alias`] that jumps to the tag under the cursor, or opens an
`fzf` menu if there are multiple matching tags:

```sh
alias xtag 'exec -s -o eval -e errmsg $DTE_HOME/scripts/xtag.sh $WORD'
```

## Quick `dterc(5)` documentation lookup

Create an [`alias`] that runs `man dterc` and jumps directly to the part
describing a specific command:

```sh
alias help 'exec $DTE_HOME/scripts/help.sh'
alias example1 'help insert'
alias example2 'help exec'
```

## Paste from system clipboard

Create an [`alias`] that inserts the contents of the system clipboard into
the buffer (using `wl-paste`, `xclip`, `pbpaste`, etc., so no SSH support):

```sh
alias cbpaste 'exec -ms -o buffer -e errmsg $DTE_HOME/scripts/paste.sh'
```

*Note:* the above examples assume you've copied the scripts in question
to `$DTE_HOME/scripts/` (i.e. `~/.dte/scripts/`).

## Contributing

Feel free to send [merge requests] adding new scripts here. Using `sh`
or `awk` as the script interpreter is a plus (for portability), but
isn't a requirement.


[`alias`]: https://craigbarnes.gitlab.io/dte/dterc.html#alias
[`compile`]: https://craigbarnes.gitlab.io/dte/dterc.html#compile
[`exec`]: https://craigbarnes.gitlab.io/dte/dterc.html#exec
[merge requests]: https://gitlab.com/craigbarnes/dte/-/merge_requests
