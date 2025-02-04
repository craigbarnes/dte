#!/usr/bin/awk -f
# This reformats gcovr's text output, so as to omit the "missing" column

FNR <= 3 {
    next # Skip the first 3 lines
}

/^---/ {
    print $0
    next
}

{
    printf("%-35s %10s %10s %10s\n", $1, $2, $3, $4)
}
