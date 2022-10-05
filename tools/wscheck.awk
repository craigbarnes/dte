#!/usr/bin/awk -f

FNR == 1 {
    printf(" %7s  %s\n", "WSCHECK", FILENAME)
}

/^\t/ {
    nlines++
    print FILENAME ":" FNR ": tab indent"
}

END {
    if (nlines) {
        print "Error: tab indents found on " nlines " lines"
        exit 1
    }
}
