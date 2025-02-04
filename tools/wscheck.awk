#!/usr/bin/awk -f

BEGIN {
    t = w = n = 0
}

FNR == 1 && NR > 1 && NR - 1 == lastblank {
    n++
    print lastname ":" lastblankfnr ": blank line at EOF"
}

FNR == 1 {
    print " WSCHECK  " FILENAME
}

/^[ \t]*$/ {
    lastblank = NR
    lastblankfnr = FNR
    lastname = FILENAME
}

/^ *\t/ {
    t++
    print FILENAME ":" FNR ": tab in indent"
}

/[ \t]+$/ {
    w++
    print FILENAME ":" FNR ": trailing whitespace"
}

function plural(x) {
    return (x == 1) ? "" : "s"
}

END {
    if (NR == lastblank) {
        n++
        print FILENAME ":" FNR ": blank line at EOF"
    }
    if (t) {
        print "Error: tab in indent on " t " line" plural(t)
    }
    if (w) {
        print "Error: trailing whitespace on " w " line" plural(w)
    }
    if (n) {
        print "Error: blank line at EOF in " n " file" plural(n)
    }
    if (t + w + n != 0) {
        exit 1
    }
}
