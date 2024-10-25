#!/bin/sh

error() {
    echo "$1" >&2
    exit 1
}

if test "$#" -eq 0; then
    error "Usage: $0 TAGNAME"
fi

if ! command -v readtags >/dev/null; then
    error 'Required command "readtags" not found'
fi

tags=$(readtags "$1")
if test -z "$tags"; then
    error 'No tags found'
fi

count=$(echo "$tags" | wc -l)
if test "$count" -eq 1; then
    tag=$(echo "$tags" | cut -f1)
    echo "tag -- '$tag'"
    exit 0
fi

echo "exec -o tag sh -c \"readtags '$1' | fzf --reverse\""
