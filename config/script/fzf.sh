#!/bin/sh

# When this is run via the `eval` command, it allows the selection
# (as created with the Tab key) to be confirmed with Ctrl/Alt + h/v,
# in order to create window splits instead of simply opening files.
# The Ctrl variants `open` the first file and `wsplit` the rest, so
# that the current window is included in the split.

fzf \
    --multi \
    --expect=ctrl-v,ctrl-h,alt-v,alt-h,enter \
    --bind='f1:change-prompt(find .>)+reload(find . -type f -not -path "*/\.git/*")' \
    --bind='f2:change-prompt(git ls-files>)+reload(git ls-files)' \
    "$@" \
| awk '
    BEGIN {
        cmds["ctrl-v"] = "wsplit -h"
        cmds["ctrl-h"] = "wsplit"
        cmds["alt-v"] = "wsplit -h"
        cmds["alt-h"] = "wsplit"
        cmds["enter"] = "open"
    }

    NR == 1 {
        cmd = cmds[$1]
        first = ($1 ~ /^alt-/) ? cmd : "open"
        next
    }

    {
        printf("%s -- \047%s\047\n", NR == 2 ? first : cmd, $0)
    }
'
