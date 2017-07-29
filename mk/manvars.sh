#!/bin/sh

test -n "$1" -a -n "$2" || {
    echo "Usage: $0 INFILE PROGNAME" >&2
    exit 1
}

MAN="$(basename ${1%.*} | tr a-z A-Z)"

sed "s/%MAN%/$MAN/g; s/%PROGRAM%/$2/g" "$1"
