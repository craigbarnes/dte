#!/bin/sh
set -eu

for cmd_ in nm awk grep; do
    if ! command -v "$cmd_" >/dev/null; then
        echo "$0:$LINENO: Required command '$cmd_' not found" >&2
        exit 1
    fi
done

undef_syms=$(nm -P -u "$@" | awk '$2 {print $1}')

# For each object in "$@", list all extern functions that are never
# referenced elsewhere
for obj in "$@"; do
    nm -P -g "$obj" | awk '$2 == "T" {print $1}' | while read -r sym; do
        case "$sym" in
        main|str_to_uintmax|__asan_*)
            continue ;;
        esac

        if printf '%s\n' "$undef_syms" | grep -qx "$sym"; then
            continue
        fi

        echo "$obj: $sym() could be made static"
    done
done
