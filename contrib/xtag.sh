#!/bin/sh

if test "$#" -eq 0; then
    echo "Usage: $0 TAGNAME" >&2
    exit 1
fi

if ! command -v readtags >/dev/null; then
    echo 'Required command "readtags" not found' >&2
    exit 1
fi

tags=$(readtags "$1")
if test -z "$tags"; then
    echo 'No tags found' >&2
    exit 1
fi

count=$(echo "$tags" | wc -l)
if test "$count" -eq 1; then
    tag=$(echo "$tags" | cut -f1)
    echo "tag -- '$tag'"
    exit 0
fi

echo "exec -o tag sh -c \"readtags '$1' | fzf --reverse\""
