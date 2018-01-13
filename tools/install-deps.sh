#!/bin/sh
set -e

error() {
    echo "Error: $1" >&2
    exit 1
}

case "$(uname)" in
Linux)
    case $(. /etc/os-release && echo "$ID") in
    alpine)
        apk --update add make gcc binutils ncurses ncurses-dev libc-dev;;
    arch)
        pacman -S --needed --noconfirm make gcc ncurses;;
    centos|fedora)
        yum -y install make gcc binutils ncurses-devel;;
    debian|ubuntu)
        apt-get update && apt-get -qy install make gcc libncurses5-dev;;
    *)
        error 'Unrecognized Linux distro; install GNU Make, GCC and ncurses manually';;
    esac;;
OpenBSD)
    pkg_add -IU gmake gcc;;
FreeBSD)
    pkg install gmake gcc;; # TODO: test this
*)
    error 'Unrecognized OS; install GNU Make, GCC and ncurses manually';;
esac
