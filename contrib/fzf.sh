#!/bin/sh

# When this is run via the `eval` command, it allows the selection
# (as created with the Tab key) to be confirmed with Ctrl+h or Ctrl+v,
# in order to create window splits instead of simply opening files

fzf \
    --reverse \
    --multi \
    --expect=ctrl-v,ctrl-h,enter \
    --bind='f1:change-prompt(find .>)+reload(find . -type f -not -path "*/\.git/*")' \
    --bind='f2:change-prompt(git ls-files>)+reload(git ls-files)' \
| awk '
    function gencmd(cmd, filename) {
        return cmd " -- \047" filename "\047"
    }

    BEGIN {
        cmds["ctrl-v"] = "wsplit -h"
        cmds["ctrl-h"] = "wsplit"
        cmds["enter"] = "open"
    }

    NR == 1 {
        key = $1
        next
    }

    NR == 2 {
        print gencmd("open", $0)
        next
    }

    {
        print gencmd(cmds[key], $0)
    }
'
