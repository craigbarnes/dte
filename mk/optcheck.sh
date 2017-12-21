#!/bin/sh

value="$1"
file="$2"

current=$(cat "$file" 2>/dev/null)
if test "$current" != "$value"; then
    test "$SILENT_BUILD" || printf ' %7s  %s\n' UPDATE "$file"
    echo "$value" > "$file"
fi
