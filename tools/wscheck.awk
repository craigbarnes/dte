#!/usr/bin/awk -f

FNR == 1 {
    print " WSCHECK  " FILENAME
}

/^ *\t/ {
    t++
    print FILENAME ":" FNR ": tab in indent"
}

/[ \t]+$/ && FILENAME !~ /\.md$/ {
    w++
    print FILENAME ":" FNR ": trailing whitespace"
}

FILENAME ~ /\.[ch]$/ && FNR != 1 && /^[ \t]*#include[ \t]+"compat.h"/ {
    h++
    print FILENAME ":" FNR ": \"compat.h\" include not on line 1"
}

function plural(n) {
    return (n == 1) ? "" : "s"
}

END {
    if (t) {
        print "Error: tab in indent on " t " line" plural(t)
    }
    if (w) {
        print "Error: trailing whitespace on " w " line" plural(w)
    }
    if (h) {
        print "Error: " h " misplaced \"compat.h\" include" plural(h)
    }
    if (t + w + h != 0) {
        exit 1
    }
}
