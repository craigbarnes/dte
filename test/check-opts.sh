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

if test "$#" != 3; then
    abort 'exactly 3 arguments required; see GNUmakefile'
fi

dte="$1"
ver="$2"
logfile="$3"

test -x "$dte" || abort "argument #1 ('$dte') not executable"

check_str "$($dte -V | head -n1)" "dte $ver"
check_str "$($dte -h | head -n1)" "Usage: $dte [OPTIONS] [[+LINE] FILE]..."
check_str "$($dte -B | grep -c '^rc$')" "1"
check_str "$($dte -b rc)" "$(cat config/rc)"

$dte -s config/syntax/dte >/dev/null


# Check headless mode ------------------------------------------------------

set +e

check_str "$(echo xyz | $dte -C 'replace y Y' | cat)" 'xYz'
check_exit "$?" 0

$dte -C 'quit 91'
check_exit "$?" 91

# `replace -c` should be ignored
check_str "$(echo xyz | $dte -C 'replace -c y Y' 2>"$logfile" | cat)" 'xyz'
check_exit "$?" 0

# `quit -p` should be ignored
check_str "$($dte -C 'insert A; open; insert _; quit -p 98' 2>"$logfile" | cat)" 'A'
check_exit "$?" 0

# `suspend` should be ignored
check_str "$($dte -C 'suspend; insert q; case' 2>"$logfile" | cat)" 'Q'
check_exit "$?" 0


# Check error handling -----------------------------------------------------

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

# Too many -t options
$dte -cquit -t1 -t2 -t3 -t4 -t5 -t6 -t7 -t8 -t9 2>/dev/null
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
