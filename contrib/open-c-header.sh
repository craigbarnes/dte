#!/bin/sh
set -eu

echo "$1" | sed -n '
    /\.c$/ {s/c$/h/p; q}
    /\.h$/ {s/h$/c/p; q}
    q
'
