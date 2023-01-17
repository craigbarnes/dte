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

END {
    status = 0
    if (t) {
        print "Error: found " t " tab indent" (t == 1 ? "" : "s")
        status = 1
    }
    if (w) {
        print "Error: trailing whitespace on " w " line" (w == 1 ? "" : "s")
        status = 1
    }
    exit status
}
