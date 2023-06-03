#!/bin/sh

if test "$#" -lt 1; then
    echo "Usage: $0 SYMBOL-NAME [OBJECT-OR-BINARY]" >&2
    echo "Example: $0 buf_umax_to_hex_str ./dte" >&2
    exit 64
fi

opts=''
if test -t 1; then
    if test "$COLORTERM" = truecolor; then
        opts="$opts --visualize-jumps=extended-color"
    else
        opts="$opts --visualize-jumps=color"
    fi
fi

if test "$(uname -m)" = x86_64; then
    opts="$opts -Mintel --section=.text"
fi

exec "${OBJDUMP:-objdump}" \
    --source \
    --show-all-symbols \
    --no-show-raw-insn \
    --disassembler-color=terminal \
    $opts \
    --disassemble="$1" \
    "${2:-dte}"
