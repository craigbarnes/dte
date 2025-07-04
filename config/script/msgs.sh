#!/bin/sh
set -eu

MSGPOS="$1" # Current `msg` index in dte
shift # Additional arguments are appended to the fzf(1) command below

AWK_PREVIEW_GENERATOR='
    function max(a, b) {
        return (a > b) ? a : b
    }

    BEGIN {
        split(fzf_line, fields, ": *")
        target_lineno = fields[3]
        height = ENVIRON["FZF_PREVIEW_LINES"]
        start = max(1, target_lineno - (height / 2))
        end = start + height
    }

    NR == target_lineno {
        print "\033[1;48;5;24m" $0 "\033[0m"
        next
    }

    NR >= start && NR < end
'

AWK_FILENAME_EXTRACTOR='
    BEGIN {FS = ": *"}
    NR == 1 {print $2}
'

# fzf expands {} in the --preview=* argument to the selected line, which
# in this case is produced by dte's `exec -i msg` command, in the format:
# MSG_ID: FILENAME:LINENO:COLNO: MSG_TEXT
PREVIEW="
    awk -v fzf_line={} '$AWK_PREVIEW_GENERATOR' \
    \"\$(echo {} | awk '$AWK_FILENAME_EXTRACTOR')\"
"

exec fzf \
    --with-nth 2.. \
    --sync \
    --bind "start:pos($MSGPOS)" \
    --preview="$PREVIEW" \
    "$@"
