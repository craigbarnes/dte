#!/bin/sh
# The output of this script is used to generate build/gen/platform.mk

set -u
NPROC="$(mk/nproc.sh)"
echo "NPROC = ${NPROC:-1}"

case "$(uname -s)" in
Darwin)
    echo 'LDLIBS_ICONV = -liconv' ;;
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
