#!/usr/bin/awk -f

function warn(msg) {
    print FILENAME ":" FNR ": " msg
}

function plural(n) {
    return (n == 1) ? "" : "s"
}

function include_guard_name(path) {
    gsub(/^src\//, "", path)
    gsub(/[\/.-]/, "_", path)
    return toupper(path)
}

BEGIN {
    h = H = 0
}

FNR == 1 {
    ifndef_line = -1
    print "  HCHECK  " FILENAME
}

FILENAME ~ /\.[ch]$/ && FNR != 1 && /^[ \t]*#include[ \t]+"feature.h"/ {
    h++
    warn("\"feature.h\" include not on line 1")
}

/^#ifndef / && FILENAME ~ /\.h$/ && ifndef_line == -1 {
    ifndef_line = FNR
    ifndef_name = $2
    x = include_guard_name(FILENAME)
    if (x != ifndef_name) {
        H++
        warn("include guard doesn't correspond to filename: " $2 " (expected: " x ")")
    }
}

/^#define / && FILENAME ~ /\.h$/ && FNR == ifndef_line + 1 {
    if ($2 != ifndef_name) {
        H++
        warn("#define name doesn't match #ifndef on previous line")
    }
}

END {
    if (h) {
        print "Error: " h " misplaced \"feature.h\" include" plural(h)
    }
    if (H) {
        print "Error: " H " unexpected header guard" plural(H)
    }
    if (h + H != 0) {
        exit 1
    }
}
