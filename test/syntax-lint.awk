#!/usr/bin/awk -f

# This script finds all destination state arguments in syntax files
# that can be replaced with the special state name "this". Using
# "this" is faster and usually also shorter and clearer.

function print_error(message) {
    printf("%s:%d: %s\n", FILENAME, FNR, message)
    exitcode = 1
}

function print_error_selfref() {
    print_error("Replace destination name '" STATE "' with 'this'")
}

/^state / {
    STATE = $2
    next
}

$1 == "eat" && $2 == STATE {
    print_error_selfref()
    next
}

$1 ~ /^(char|str|bufis)$/ && $3 ~ "(^|:)" STATE "$" {
    print_error_selfref()
    next
}

$1 ~ /^(char|str|bufis)$/ && $2 ~ /^-[bni]+$/ && $4 ~ "(^|:)" STATE "$" {
    print_error_selfref()
    next
}

$1 == "noeat" && ($2 == STATE || ($2 == "-b" && $3 == STATE)) {
    print_error("The 'noeat' action must jump to a different destination state")
    next
}

/^ *\t/ {
    print_error("Use 4 spaces for indents, not tabs")
    next
}

END {
    exit exitcode
}
