#!/usr/bin/awk -f

# This script finds all destination state arguments in syntax files
# that can be replaced with the special state name "this". Using
# "this" is faster and usually also shorter and clearer.

function print_match() {
    print FILENAME ":" FNR ": " $0
}

/^state / {
    STATE = $2
    next
}

$1 == "eat" && $2 == STATE {
    print_match()
    next
}

$1 == "noeat" && ($2 == STATE || ($2 == "-b" && $3 == STATE)) {
    print_match()
    next
}

$1 ~ /^(char|str|bufis)$/ && $3 ~ "(^|:)" STATE "$" {
    print_match()
    next
}

$1 ~ /^(char|str|bufis)$/ && $2 ~ /^-[bni]+$/ && $4 ~ "(^|:)" STATE "$" {
    print_match()
    next
}
