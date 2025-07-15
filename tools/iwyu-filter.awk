#!/usr/bin/awk -f

/^(include-what-you-use|.*ERROR):/ {
    print > "/dev/stderr"
}

/ should remove these lines:$/ {
    file = $1
}

/^- *#include .* \/\/ lines [0-9]+-[0-9]+$/ {
    ref = "// lines "
    start = index($0, ref) + length(ref)
    end = index(substr($0, start), "-") - 1
    lineno = substr($0, start, end)
    print file ":" lineno ": unused " $3 " include"
}
