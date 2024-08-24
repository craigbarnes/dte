#!/bin/sh

set -eu
NAME="HAVE_$1"
test "$2"
shift 1

if "$@"; then
    echo "#define $NAME 1"
else
    echo "#define $NAME 0"
fi
