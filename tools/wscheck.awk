#!/usr/bin/awk -f

BEGIN {
    t = w = n = b = 0
}

FNR == 1 && NR > 1 && NR - 1 == lastblank {
    n++
    print lastname ":" lastblankfnr ": blank line at EOF"
}

FNR == 1 {
    lastblankfnr = 0
    print " WSCHECK  " FILENAME
}

/^[ \t]*$/ {
    if (lastblankfnr > 0 && lastblankfnr == FNR - 1 && FILENAME !~ /\.md$/) {
        b++
        print FILENAME ":" FNR ": excess blank line"
    }
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
    if (b) {
        print "Error: " b " excess blank line" plural(b)
    }
    if (t + w + n + b != 0) {
        exit 1
    }
}
