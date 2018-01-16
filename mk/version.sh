#!/bin/sh

# This script generates the version string, as printed by "dte -V".

set -eu
export VPREFIX="$1"

git_describe_ver=$(git describe --match="v$VPREFIX" 2>/dev/null || true)

if test -n "$git_describe_ver"; then
    echo "$git_describe_ver" | sed 's/^v//'
    exit 0
fi

distinfo_ver=$(awk '
    NR==1 && /^[a-f0-9]{40}$/ {
        print ENVIRON["VPREFIX"] "-" substr($0,0,12) "-dist"
    }
' .distinfo)

if test -n "$distinfo_ver"; then
    echo "$distinfo_ver"
    exit 0
fi

echo "$VPREFIX-unknown"
