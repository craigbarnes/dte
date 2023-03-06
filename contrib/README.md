dte Contributed Scripts
=======================

This directory contains various example scripts that are intended to
be used in conjunction with the [`exec`] command. Feel free to send
pull requests adding new scripts here. Using `sh` or `awk` as the
script interpreter is a plus (for portability reasons), although
it's not a strict requirement.

Usage Examples
--------------

Bind Alt+h to open the `*.h` file matching the current `*.c` file
(or vice versa):

    bind M-h 'exec-open -s $DTE_HOME/scripts/open-c-header.sh $FILE'

Create an [`alias`] that jumps to the locations of all unstaged
changes made to the git working tree:

    errorfmt gitdiff '^([^:]+): [+-]([0-9]+).*' file line
    alias git-changes 'compile -1s gitdiff $DTE_HOME/scripts/git-changes.sh'

*Note:* the above examples assume you've copied the scripts in
question to `$DTE_HOME/scripts/`.


[`alias`]: https://craigbarnes.gitlab.io/dte/dterc.html#alias
[`exec`]: https://craigbarnes.gitlab.io/dte/dterc.html#exec
