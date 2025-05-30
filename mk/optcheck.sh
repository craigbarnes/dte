#!/bin/sh
set -u

# Check if STRING is equal to the contents of FILE and, if not, update it
# by writing the new string (also printing "UPDATE ..." to stdout). This
# is used in `mk/build.mk` as an aid to tracking non-file dependencies
# (typically variables) as files, for the benefit of GNU Make's dependency
# graph.

usage () {
    printf 'Usage: %s [-hsR] STRING FILE\n' "$1"
}

silent=
noregen=
while getopts 'hsR' flag; do
   case "$flag" in
   R) noregen=1 ;; # Generate FILE only if it doesn't already exist
   s) silent=1 ;; # Don't print "UPDATE ..." when FILE is updated
   h) usage "$0"; exit 0 ;;
   *) usage "$0" >&2; exit 64 ;;
   esac
done

shift $((OPTIND - 1))
value="$1"
file="$2"

if test "$noregen" && test -f "$file"; then
    # Don't update file; it already exists and -R was used
    exit 0
fi

current=$(cat "$file" 2>/dev/null)
if test "$current" != "$value"; then
    test "$silent" || printf ' %7s  %s\n' UPDATE "$file"
    echo "$value" > "$file"
fi
