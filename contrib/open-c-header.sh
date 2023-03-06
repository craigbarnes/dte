#!/bin/sh
set -eu

path="$1"

case "$path" in
    *.c) echo "$path" | sed 's|\.c$|.h|' ;;
    *.h) echo "$path" | sed 's|\.h$|.c|' ;;
esac
