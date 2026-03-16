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

        if (fields[3] ~ /^\/\^.*\$\/$/) {
            # Line in `MSG_ID: FILENAME: /^PATTERN$/` format. Remove the
            # first and last 2 chars and try to find a matching line in
            # the file, in order to determine its line number.
            filename = fields[2]
            pattern = fields[3]
            target_line_text = substr(pattern, 3, length(pattern) - 4)
            target_lineno = 0
            n = 0

            while ((getline line < filename) > 0) {
                n++
                # Note that any backslash-escaped special chars in the original
                # pattern have been converted (by `awk -v`) to literal chars,
                # which was the point of escaping them anyway, so we simply
                # match the literal string contents instead of regex matching.
                if (line == target_line_text) {
                    target_lineno = n
                    break
                }
            }

            close(filename)
            if (target_lineno == 0) {
                printf("Error: target line not found:\n\n%s\n", target_line_text)
                exit 1
            }
        } else if (fields[3] ~ /^[0-9]+$/) {
            # Line in `MSG_ID: FILENAME:LINENO:[COLNO:] MSG_TEXT` format.
            # Use the LINENO field as provided.
            target_lineno = fields[3]
        } else {
            # No LINENO provided or unknown message format. Just exit and
            # leave the preview window blank.
            exit 0
        }

        height = ENVIRON["FZF_PREVIEW_LINES"]
        start = max(1, target_lineno - (height / 2))
        end = start + height
    }

    NR == target_lineno {
        print "\033[1;48;5;24m" $0 "\033[K\033[0m"
        next
    }

    NR >= start && NR < end
'

AWK_FILENAME_EXTRACTOR='
    BEGIN {FS = ": *"}
    NR == 1 {print $2}
'

# fzf expands {} in the --preview=* argument to the selected line, which
# in this case is produced by dte's `exec -i msg` command, in one of the
# following formats:
# • `MSG_ID: [FILENAME:][LINENO:][COLNO:] MSG_TEXT`
# • `MSG_ID: FILENAME: /^PATTERN$/`
PREVIEW="
    awk -v fzf_line={} '$AWK_PREVIEW_GENERATOR' \
    \"\$(echo {} | awk '$AWK_FILENAME_EXTRACTOR')\" 2>/dev/null
"

export POSIXLY_CORRECT=1 # See gawk(1)

exec fzf \
    --with-nth 2.. \
    --sync \
    --bind "start:pos($MSGPOS)" \
    --preview="$PREVIEW" \
    "$@"
