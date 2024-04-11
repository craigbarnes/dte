#!/usr/bin/awk -f

FNR == 1 {
    print " WSCHECK  " FILENAME
}

/^ *\t/ {
    t++
    print FILENAME ":" FNR ": tab in indent"
}

/[ \t]+$/ {
    w++
    print FILENAME ":" FNR ": trailing whitespace"
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
    if (t + w != 0) {
        exit 1
    }
}
