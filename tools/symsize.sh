#!/bin/sh

# This script filters output from "nm" to show the sizes of symbols
# (in KiB) in an object file, sorted from largest to smallest.

nm -Ptd ${@:-dte} | awk '
{
    name = $1
    type = $2
    size = $4

    if (size < 128) {
        next
    }

    ksize = sprintf("%.2f", $4 / 1024)
    printf(" %s %6sK  %s\n", type, ksize, name)
}
' | sort -k2 -nr
