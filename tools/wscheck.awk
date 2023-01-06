#!/usr/bin/awk -f

FNR == 1 {
    print " WSCHECK  " FILENAME
}

/^ *\t/ {
    t++
    print FILENAME ":" FNR ": tab in indent"
}

END {
    if (t) {
        print "Error: found " t " tab indent" (t == 1 ? "" : "s")
        exit 1
    }
}
