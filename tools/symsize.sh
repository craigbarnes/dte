#!/bin/sh

# This script filters output from nm(1), to show the sizes of symbols
# (in KiB) in an object file, sorted from largest to smallest.

if test ! -t 1; then
    PAGER='cat'
fi

${NM:-nm} -Ptd "${@:-dte}" | sort -k4 -nr | awk '
{
    name = $1
    type = $2
    size = $4

    if (size < 1 || type ~ /[Bb]/) {
        next
    } else if (size < 10) {
        printf(" %s %7d  %s\n", type, size, name)
        next
    }

    ksize = sprintf("%.2f", size / 1024)
    printf(" %s %6sK  %s\n", type, ksize, name)
}
' | ${PAGER:-less}
