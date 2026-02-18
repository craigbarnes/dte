#!/bin/sh
set -eu

# This script generates the version string, as printed by "dte -V".

# These `$Format$` strings are filled automatically for git-archive(1)
# tarballs, but remain as-is in a cloned git repository. The tag name
# specified by `describe:match=` must appear as literal text in order
# to be expanded, so it must be updated in both of the variables below
# after each release.
# See also:
# • "export-subst" in git-archive(1) and gitattributes(5)
# • The output of e.g. `git show -s --format='%(describe:match=v1.11.1)' d9c416ee49c3`
# • docs/releasing.md
distinfo_commit_short='$Format:%h$'
distinfo_describe_ver='$Format:%(describe:match=v1.11.1)$'
VPREFIX='1.11.1'

if expr "$distinfo_describe_ver" : "v${VPREFIX}-[0-9][0-9]*-g" >/dev/null; then
    echo "${distinfo_describe_ver#v}-dist"
    exit 0
fi

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
