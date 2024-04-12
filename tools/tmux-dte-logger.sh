#!/bin/sh
set -eu

# This script starts ./dte with logging sent to a new tmux(1) window
# split and then closes the split when dte exits (unless -K is used)

usage () {
    printf 'Usage: %s [-Kt] [DTE-ARGS]...\n' "$0"
}

if ! command -v tmux >/dev/null; then
    echo "$0:${LINENO:-2}: Required command 'tmux' not found" >&2
    exit 127
fi

# Exit (due to `set -u`) if not inside a tmux session
test "$TMUX"

killpane=1
while getopts 'Kth' flag; do
   case "$flag" in
   K) killpane= ;;
   t) export DTE_LOG_LEVEL=trace ;;
   h) usage; exit 0 ;;
   *) usage >&2; exit 64 ;;
   esac
done

shift $((OPTIND - 1))

info="$(tmux splitw -hdP -F '#{pane_tty},#{pane_id}' cat -v)"
pane_tty="$(echo $info | cut -d, -f1)"
pane_id="$(echo $info | cut -d, -f2)"

DTE_LOG="$pane_tty" ${DTE:-./dte} "$@"

if test "$killpane"; then
    exec tmux kill-pane -t "$pane_id"
fi
