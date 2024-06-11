#!/bin/sh

case "$(uname)" in
Linux)
    # getconf(1) is usually provided by glibc and isn't present on Android (Termux).
    # nproc(1) is from coreutils, which may not be installed on some distros.
    (getconf _NPROCESSORS_ONLN || nproc || echo 1) 2>/dev/null;;
FreeBSD|NetBSD|OpenBSD|DragonFly)
    exec /sbin/sysctl -n hw.ncpu;;
Darwin)
    exec /usr/sbin/sysctl -n hw.activecpu;;
SunOS)
    exec /usr/sbin/psrinfo -p;;
*)
    (
        getconf NPROCESSORS_ONLN ||
        getconf _NPROCESSORS_ONLN ||
        sysctl -n hw.ncpu ||
        nproc ||
        echo 1
    ) 2>/dev/null;;
esac
