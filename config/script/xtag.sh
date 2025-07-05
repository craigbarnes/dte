#!/bin/sh

error() {
    echo "$1" >&2
    exit 1
}

if ! command -v readtags >/dev/null; then
    error 'Required command "readtags" not found'
fi

if test "$#" -lt 1; then
    error "Usage: xtag.sh TAGNAME"
fi

TAGNAME="$1"
tags=$(readtags "$TAGNAME")

if test -z "$tags"; then
    error "No tags found with name '$TAGNAME'"
fi

count=$(echo "$tags" | wc -l)
if test "$count" -eq 1; then
    tag=$(echo "$tags" | cut -f1)
    echo "tag -- '$tag'"
    exit 0
fi

echo "exec -o tag sh -c \"readtags '$TAGNAME' | fzf --reverse\""
