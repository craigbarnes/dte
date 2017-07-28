#!/bin/sh

case "$(uname)" in
Linux)
    exec getconf _NPROCESSORS_ONLN;;
FreeBSD|NetBSD|OpenBSD)
    exec /sbin/sysctl -n hw.ncpu;;
Darwin)
    exec /usr/sbin/sysctl -n hw.activecpu;;
SunOS)
    exec /usr/sbin/psrinfo -p;;
*)
    echo 1;;
esac
