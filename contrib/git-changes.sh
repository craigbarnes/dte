#!/bin/sh

git diff --relative -U0 "$@" | awk '
    /^\+{3} b\// {
        filename = substr($2, 3)
    }

    /^@@/ {
        print filename ": " $3
    }
'
