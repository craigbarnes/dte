#!/bin/sh

git diff -U0 "$@" | awk '
    /^\+{3} b\// {
        filename = substr($2, 3)
    }

    /^@@/ {
        print filename ": " $3
    }
'
