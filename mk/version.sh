#!/bin/sh

# This script generates the version string, as printed by "dte -V".

set -eu
export VPREFIX="$1"

# This value is filled automatically for git-archive(1) tarballs.
# See also: "export-subst" in gitattributes(5).
distinfo_commit_short='$Format:%h$'

if expr "$distinfo_commit_short" : '[0-9a-f]\{7,40\}$' >/dev/null; then
    echo "${VPREFIX}-g${distinfo_commit_short}-dist"
    exit 0
fi

git_describe_ver=$(git describe --match="v$VPREFIX" 2>/dev/null || true)
if test -n "$git_describe_ver"; then
    echo "${git_describe_ver#v}"
    exit 0
fi

echo "$VPREFIX-unknown"
