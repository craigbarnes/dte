#!/bin/sh
set -e

# Bash doesn't expand aliases in non-interactive shells, unless the
# expand_aliases option is enabled or it's running in "POSIX mode":
export POSIXLY_CORRECT=1

error() {
    echo "$0:$1: Error: $2" >&2
    exit 1
}

assert_streq() {
    if test "$2" != "$3"; then
        error "$1" "strings not equal: '$2', '$3'"
    fi
}

alias abort='error "$LINENO"'
alias check='assert_streq "$LINENO"'

if test "$#" != 2; then
    abort 'exactly 2 arguments required'
fi

dte="$1"
ver="$2"

test -x "$dte" || abort "argument #1 ('$dte') not executable"

check "$($dte -V | head -n1)" "dte $ver"
check "$($dte -h | head -n1)" "Usage: $dte [OPTIONS] [[+LINE] FILE]..."
check "$($dte -B | head -n1)" "rc"
check "$($dte -b rc)" "$(cat config/rc)"

$dte -s config/syntax/dte >/dev/null
