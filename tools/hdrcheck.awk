#!/usr/bin/awk -f

FNR == 1 {
    ifndef_fnr = -1
    print "  HCHECK  " FILENAME
}

FILENAME ~ /\.[ch]$/ && FNR != 1 && /^[ \t]*#include[ \t]+"compat.h"/ {
    h++
    print FILENAME ":" FNR ": \"compat.h\" include not on line 1"
}

/^#ifndef / && FILENAME ~ /\.h$/ && ifndef_fnr == -1 {
    if ($2 !~ /_H$/) {
        print FILENAME ":" FNR ": first #ifndef doesn't end with \"_H\""
        H++
        next
    }

    ifndef_fnr = FNR

    # Derive expected include guard name from filename
    x = FILENAME
    gsub(/^src\//, "", x)
    gsub(/[\/.-]/, "_", x)
    x = toupper(x)

    if (x != $2) {
        H++
        print FILENAME ":" FNR ": include guard doesn't correspond to filename: " $2 " (expected: " x ")"
    }
}

/^#define / && FILENAME ~ /\.h$/ && FNR == ifndef_fnr + 1 {
    if ($2 != x) {
        H++
        print FILENAME ":" FNR ": #define name doesn't match #ifndef on previous line"
    }
}

function plural(n) {
    return (n == 1) ? "" : "s"
}

END {
    if (h) {
        print "Error: " h " misplaced \"compat.h\" include" plural(h)
    }
    if (H) {
        print "Error: " H " unexpected header guard" plural(H)
    }
    if (h + H != 0) {
        exit 1
    }
}
