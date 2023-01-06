#!/usr/bin/awk -f

FNR == 1 {
    print " WSCHECK  " FILENAME
}

/^ *\t/ {
    nlines++
    print FILENAME ":" FNR ": tab in indent"
}

END {
    if (nlines) {
        print "Error: tab indents found on " nlines " lines"
        exit 1
    }
}
