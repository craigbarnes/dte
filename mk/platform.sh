#!/bin/sh
set -eu

case "$(uname -s)" in
Linux)
    exit;;
Darwin)
    echo 'LDLIBS_ICONV = -liconv'
    exit;;
OpenBSD)
    echo 'LDLIBS_ICONV = -liconv'
    echo 'BASIC_CPPFLAGS += -I/usr/local/include'
    echo 'BASIC_LDFLAGS += -L/usr/local/lib'
    exit;;
NetBSD)
    echo 'BASIC_CPPFLAGS += -I/usr/pkg/include'
    echo 'BASIC_LDFLAGS += -L/usr/pkg/lib'
    exit;;
esac

if test "$(uname -o)" = Cygwin; then
    echo 'LDLIBS_ICONV = -liconv'
    echo 'EXEC_SUFFIX = .exe'
fi
