#!/bin/sh
set -eu

TMPFILE="$(mktemp)"
trap "rm -f '$TMPFILE'" INT QUIT TERM HUP EXIT
ranger --choosefiles="$TMPFILE" --cmd='map <esc> quit' >/dev/tty
cat "$TMPFILE"
