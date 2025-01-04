#!/bin/sh
# The output of this script is used to generate build/gen/platform.mk

set -u
KERNEL="$(uname -s)"
NPROC="$(mk/nproc.sh)"

echo "# Generated by mk/platform.sh; do not edit
KERNEL = ${KERNEL:-unknown}
NPROC = ${NPROC:-1}"

# Check for `xargs -P` (parallel execution) support
# See also: https://austingroupbugs.net/view.php?id=1801
if test "$NPROC" -gt 1 && printf "1\n2" | xargs -P2 -I@ echo '@' >/dev/null; then
    echo "XARGS_P = xargs -P$NPROC"
else
    echo 'XARGS_P = xargs'
fi

NO_INSTALL_XDG_CLUTTER='
# This causes `make install` to exclude .desktop/icon/AppStream files
# See also: docs/packaging.md ("installation targets") and GNUmakefile
NO_INSTALL_XDG_CLUTTER = 1'

case "$KERNEL" in
Linux)
    OS="$(uname -o)"
    if test "$OS" = Android; then
        echo 'LDLIBS_ICONV = -liconv'
        echo "$NO_INSTALL_XDG_CLUTTER"
    fi ;;
Darwin)
    echo 'LDLIBS_ICONV = -liconv'
    echo "$NO_INSTALL_XDG_CLUTTER" ;;
OpenBSD)
    echo 'LDLIBS_ICONV = -liconv'
    echo 'BASIC_CPPFLAGS += -I/usr/local/include'
    echo 'BASIC_LDFLAGS += -L/usr/local/lib' ;;
NetBSD)
    echo 'BASIC_CPPFLAGS += -I/usr/pkg/include'
    echo 'BASIC_LDFLAGS += -L/usr/pkg/lib' ;;
CYGWIN*)
    echo 'LDLIBS_ICONV = -liconv'
    echo 'EXEC_SUFFIX = .exe' ;;
esac
