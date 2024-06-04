#!/bin/sh

if test -z "$1"; then
    exec man dterc
    exit $?
fi

pager="less --use-backslash '+/^ +$1( |\\\$)
n'"

exec man -P "$pager" dterc
