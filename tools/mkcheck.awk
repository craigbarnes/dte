#!/usr/bin/awk -f
# Basic linter for makefiles

function warn(msg) {
    print FILENAME ":" FNR ": " msg
}

function warn_prev(msg) {
    print prevfile ":" prevline ": " msg
}

function plural(n) {
    return (n == 1) ? "" : "s"
}

BEGIN {
    prevfile = prevtext = ""
    w = c = b = prevline = 0
}

# The `FNR == 1` case here is to catch bad line continations on the last
# line of a file (which don't need to be followed by a blank line to be
# bad). This test is also ordered first because it reports errors on the
# *previous* line, which should come before errors on the current line
# and/or "MKCHECK FILENAME" output.
(FNR == 1 || /^[ \t]*$/) && prevtext ~ /\\$/ {
    warn_prev("bad line continuation")
    c++
}

FNR == 1 && prevfile != "" && prevtext ~ /^[ \t]*$/ {
    warn_prev("blank line at EOF")
    b++
}

FNR == 1 {
    print " MKCHECK  " FILENAME
}

/[ \t]$/ {
    warn("trailing whitespace")
    w++
}

{
    prevfile = FILENAME
    prevline = FNR
    prevtext = $0
}

END {
    if (w) {
        print "Error: trailing whitespace on " w " line" plural(w)
    }
    if (c) {
        s = plural(c)
        print "Error: bad line continuation" s " on " c " line" s
    }
    if (b) {
        s = plural(b)
        print "Error: blank line" s " at EOF in " b " file" s
    }
    if (w + c + b) {
        exit 1
    }
}
