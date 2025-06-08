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
alias check_exit='assert_exitcode "$LINENO" "$?"'

if test "$#" != 3; then
    abort 'exactly 3 arguments required; see GNUmakefile'
fi

dte="$1"
ver="$2"
logfile="$3"

test -x "$dte" || abort "argument #1 ('$dte') not executable"

check_str "$($dte -V | head -n1)" "dte $ver"
check_str "$($dte -B | grep -c '^rc$')" "1"
check_str "$($dte -b rc)" "$(cat config/rc)"

echo "$dte -V: $ver" >"$logfile" # Clear previous log contents
$dte -Z >>"$logfile"
$dte -s config/syntax/dte >>"$logfile"
$dte -h >/dev/null
$dte -P >/dev/null


# Check headless mode ------------------------------------------------------

set +e

out="$(echo xyz | $dte -HRC 'replace y Y' | cat)"
check_exit 0
check_str "$out" 'xYz'

out="$($dte -HRC 'quit 91')"
check_exit 91
check_str "$out" ''

out="$($dte -H -Q0 -r/dev/null -C 'quit 77')"
check_exit 77
check_str "$out" ''

# `replace -c` should be ignored
out="$(echo xyz | $dte -HRC 'replace -c y Y' 2>>"$logfile" | cat)"
check_exit 0
check_str "$out" 'xyz'

# `quit -p` should be ignored
out="$($dte -HRC 'insert A; open; insert _; quit -p 98' 2>>"$logfile" | cat)"
check_exit 0
check_str "$out" 'A'

# `suspend` should be ignored
out="$($dte -HRC 'suspend; insert q; case' 2>>"$logfile" | cat)"
check_exit 0
check_str "$out" 'Q'

# Regression test for commit 7cf2b19320a4c2
out="$($dte -Hr test/check-opts.dterc -C 'ins42' | cat)"
check_exit 0
check_str "$out" '42'
out="$($dte -HRr test/check-opts.dterc -C 'ins42' 2>>"$logfile" | cat)"
check_exit 0
check_str "$out" ''

# Check error handling -----------------------------------------------------

# Unknown option (error message is from getopt(3), so not checked)
$dte -7 2>>"$logfile"
check_exit 64

# Missing option argument (error message is from getopt(3), so not checked)
$dte -r 2>>"$logfile"
check_exit 64

# Linting invalid syntax file
err="$($dte -s /dev/null 2>&1)"
check_exit 65
check_str "$err" '/dev/null: no default syntax found'

# Dumping non-existent builtin config
err="$($dte -b non-existent 2>&1)"
check_exit 64
check_str "$err" "Error: no built-in config with name 'non-existent'"

# Too many -c/-C options
err="$($dte -C1 -C2 -C3 -C4 -C5 -c6 -c7 -c8 -c9 2>&1)"
check_exit 64
check_str "$err" 'Error: too many -c or -C options used'

# Too many -t options
err="$($dte -Cquit -t1 -t2 -t3 -t4 -t5 -t6 -t7 -t8 -t9 2>&1)"
check_exit 64
check_str "$err" 'Error: too many -t options used'

# Invalid -Q argument
err="$($dte -Qxyz 2>&1)"
check_exit 64
check_str "$err" "Error: invalid argument for -Q: 'xyz'"

if ! command -v setsid >/dev/null; then
    if test "$(uname -s)" = "Linux"; then
        abort "setsid(1) required; install util-linux"
    fi
    exit 0
fi

# No controlling tty
# shellcheck disable=SC2086
TERM=xterm-256color setsid $dte -HR -cquit </dev/null >/dev/null 2>>"$logfile"
check_exit 74
