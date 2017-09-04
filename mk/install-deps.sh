#!/bin/sh
set -e

linux_distro() {
    case $(. /etc/os-release && echo "$ID") in
    alpine)
        apk --update add make gcc binutils ncurses ncurses-dev libc-dev;;
    arch)
        pacman -S --needed --noconfirm make gcc ncurses;;
    centos|fedora)
        yum -y install make gcc binutils ncurses-devel;;
    debian|ubuntu)
        apt-get update && apt-get -qy install make gcc binutils libncurses5-dev;;
    *)
        echo 'Unrecognized Linux distro; install GNU Make, GCC and ncurses manually';;
    esac
}

case $(uname) in
Linux)
    linux_distro;;
OpenBSD)
    pkg_add -IU gmake gcc;;
FreeBSD)
    pkg install gmake gcc;; # TODO: test this
Darwin)
    ;;
*)
    echo 'Unrecognized OS; install GNU Make, GCC and ncurses manually';;
esac
