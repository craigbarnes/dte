#!/usr/bin/awk -f

BEGIN {
    lineno = 1
    maxlen = 0
}

{
    len = length($0)
    if (len > maxlen) {
        maxlen = len
        lineno = FNR
    }
}

END {
    print "line " lineno
}
