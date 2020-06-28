#!/bin/sh

case "$(uname)" in
Linux)
    exec getconf _NPROCESSORS_ONLN;;
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
