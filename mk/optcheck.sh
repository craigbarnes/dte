#!/bin/sh

value="$1"
file="$2"

current=$(cat "$file" 2>/dev/null)
if test "$current" != "$value"; then
    echo "  UPDATE  $file"
    echo "$value" > "$file"
fi
