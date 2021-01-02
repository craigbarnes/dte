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
        apk --update add make gcc binutils libc-dev;;
    arch|manjaro|artix)
        pacman -S --needed --noconfirm make gcc;;
    centos|fedora)
        yum -y install make gcc binutils;;
    debian|ubuntu|linuxmint)
        apt-get update && apt-get -qy install make gcc;;
    void)
        xbps-install -S make gcc;;
    *)
        error 'Unrecognized Linux distro; install GNU Make and GCC manually';;
    esac;;
OpenBSD)
    pkg_add -IU gmake gcc;;
NetBSD)
    pkg_add gmake gcc;;
FreeBSD|DragonFly)
    pkg install gmake gcc;;
*)
    error 'Unrecognized OS; install GNU Make and GCC manually';;
esac
