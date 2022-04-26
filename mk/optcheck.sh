#!/bin/sh
set -u

usage () {
    printf 'Usage: %s [-hsR] STR FILE\n' "$1"
}

silent=
noregen=
while getopts 'hsR' flag; do
   case "$flag" in
   R) noregen=1 ;; # Generate FILE only if it doesn't already exist
   s) silent=1 ;; # Don't print any output when FILE is updated
   h) usage "$0"; exit 0 ;;
   *) usage "$0" >&2; exit 64 ;;
   esac
done

shift $((OPTIND - 1))
value="$1"
file="$2"

if test "$noregen" -a -f "$file"; then
    # Don't update file; it already exists and -R was used
    exit 0
fi

current=$(cat "$file" 2>/dev/null)
if test "$current" != "$value"; then
    test "$silent" || printf ' %7s  %s\n' UPDATE "$file"
    echo "$value" > "$file"
fi
