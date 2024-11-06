#!/bin/sh
set -e

if test "$#" -gt 1 || test -z "$(size -V 2>/dev/null | head -n1 | grep '^GNU ')"; then
    size "${@:-dte}"
    exit $?
fi

size -A "${1:-dte}" | sort -nrk2 | awk '
    $2 ~ /^[0-9]{3}/ {
        printf("%8.1fK  %s\n", $2 / 1024, $1)
    }
'
