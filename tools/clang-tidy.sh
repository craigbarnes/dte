#!/bin/sh
set -eu

test "$#" -gt 0 || {
    echo "Usage: $0 FILE..."
    exit 64
}

# See also: -Wno-unterminated-string-initialization comment in mk/compiler.sh
CFLAGS='
    -std=gnu11
    -O2
    -Wall
    -Wextra
    -Wundef
    -Wcomma
    -Wno-unterminated-string-initialization
    -DDEBUG=3
    -D_FILE_OFFSET_BITS=64
    -Isrc
    -Itools/mock-headers
'

# shellcheck disable=SC2086
${CLANGTIDY:-clang-tidy} "$@" -- $CFLAGS 2>&1 |
    sed -E '/^[0-9]+ warnings? .*generated\.$/d' >&2
