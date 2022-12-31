#!/bin/sh

export POSIXLY_CORRECT=1

error() {
    echo "$0:$1: Error: $2" >&2
    exit 1
}

assert_exitcode() {
    if ! test "$2" -eq "$3"; then
        error "$1" "exit codes not equal: $2, $3"
    fi
}

alias abort='error "$LINENO"'
alias check='assert_exitcode "$LINENO"'

if test "$#" != 1; then
    abort 'exactly 1 argument required'
fi

dte="$1"

# Unknown option
$dte -7 2>/dev/null
check "$?" 64

# Missing option argument
$dte -r 2>/dev/null
check "$?" 64

# Linting invalid syntax file
$dte -s /dev/null 2>/dev/null
check "$?" 65

# Dumping non-existent builtin config
$dte -b non-existent 2>/dev/null
check "$?" 64

# Too many -c options
$dte -cquit -c2 -c3 -c4 -c5 -c6 -c7 -c8 -c9 2>/dev/null
check "$?" 64

# Unset $TERM
TERM= $dte -cquit 2>/dev/null
check "$?" 64

if ! command -v setsid >/dev/null; then
    if test "$(uname -s)" = "Linux"; then
        echo "$0:$LINENO: setsid(1) required; install util-linux" >&2
        exit 1
    fi
    exit 0
fi

# No controlling tty
TERM=xterm-256color setsid $dte -cquit >/dev/null 2>&1 0>&1
check "$?" 74
