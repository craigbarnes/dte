#!/bin/sh
set -e
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

assert_exitcode() {
    if ! test "$2" -eq "$3"; then
        error "$1" "exit codes not equal: $2, $3"
    fi
}

alias abort='error "$LINENO"'
alias check_str='assert_streq "$LINENO"'
alias check_exit='assert_exitcode "$LINENO"'

if test "$#" != 2; then
    abort 'exactly 2 arguments required'
fi

dte="$1"
ver="$2"

test -x "$dte" || abort "argument #1 ('$dte') not executable"

check_str "$($dte -V | head -n1)" "dte $ver"
check_str "$($dte -h | head -n1)" "Usage: $dte [OPTIONS] [[+LINE] FILE]..."
check_str "$($dte -B | grep -c '^rc$')" "1"
check_str "$($dte -b rc)" "$(cat config/rc)"

$dte -s config/syntax/dte >/dev/null

# Check error handling -----------------------------------------------------
set +e

# Unknown option
$dte -7 2>/dev/null
check_exit "$?" 64

# Missing option argument
$dte -r 2>/dev/null
check_exit "$?" 64

# Linting invalid syntax file
$dte -s /dev/null 2>/dev/null
check_exit "$?" 65

# Dumping non-existent builtin config
$dte -b non-existent 2>/dev/null
check_exit "$?" 64

# Too many -c options
$dte -cquit -c2 -c3 -c4 -c5 -c6 -c7 -c8 -c9 2>/dev/null
check_exit "$?" 64

# Unset $TERM
TERM='' $dte -cquit 2>/dev/null
check_exit "$?" 64

if ! command -v setsid >/dev/null; then
    if test "$(uname -s)" = "Linux"; then
        abort "setsid(1) required; install util-linux"
    fi
    exit 0
fi

# No controlling tty
# shellcheck disable=SC2086
TERM=xterm-256color setsid $dte -cquit >/dev/null 2>&1 0>&1
check_exit "$?" 74
