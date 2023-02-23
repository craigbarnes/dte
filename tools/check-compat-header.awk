#!/usr/bin/awk -f

BEGIN {
    if (ENVIRON["VERBOSE"] == 1) {
        verbose = 1
    }
}

/^[ \t]*#include[ \t]+"compat.h"/ {
    if (FNR != 1) {
        print FILENAME ":" FNR ": Error: #include \"compat.h\" must be on line 1"
        errors++
    }
    if (verbose) {
        print FILENAME ":" FNR ": " $0
    }
}

END {
    if (errors) {
        exit 1
    }
}
