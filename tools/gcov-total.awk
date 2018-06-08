#!/usr/bin/awk -f

/has arcs (to entry|from exit) block/ {
    next
}

/^Lines executed:[0-9.]+% of [0-9]+/ {
    match($0, /[0-9.]+%/)
    percent = substr($0, RSTART, RLENGTH - 1)
    sum += percent
    files += 1
}

{
    print
}

END {
    CONVFMT = "%.4g"
    total = sum / files
    print "\nTotal lines executed:: " total "%\n"
}
