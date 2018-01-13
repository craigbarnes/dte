#!/bin/sh
set -e

PROGNAME="$0"

abort() {
    echo "$PROGNAME:$2: Error: $1" >&2
    exit 1
}

test "$1" || abort 'one or more arguments are required' $LINENO
command -v size >/dev/null || abort '"size" command not found' $LINENO

if test -n "$2" -o -z "$(size -V 2>/dev/null | head -n1 | grep '^GNU ')"; then
    size $@
    exit $?
fi

test -f "$1" || abort "no such file '$1'" $LINENO

size -A "$1" | sort -nrk2 | awk '
    $2 ~ /^[0-9]{3}/ {
        printf("%8.1fK  %s\n", $2 / 1024, $1)
    }
'
