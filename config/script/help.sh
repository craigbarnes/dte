#!/bin/sh

if test -z "$1"; then
    exec man dterc
    exit $?
fi

# Use a custom man pager, to jump directly to the part of dterc(5) that
# describes the command matching $1
pager="less --use-backslash '+/^ +$1( |\\\$)
n'"

# If $1 is "alias", run the less(1) "n" command twice instead of just
# once, so as to skip past "alias" in SYNOPSIS and also in the usage
# example under DESCRIPTION
case "$1" in
    alias) pager="${pager}n" ;;
esac

exec man -P "$pager" dterc
