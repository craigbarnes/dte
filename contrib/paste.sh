#!/bin/sh

# Most terminals don't allow pasting via OSC 52, but a command like e.g.
# wl-paste(1) can be used instead, if the editor is running on the same
# machine as the terminal.

havecmd() {
    command -v "$1" >/dev/null
}

case "$(uname)" in
Darwin)
    exec pbpaste ;;
CYGWIN*)
    exec cat /dev/clipboard ;;
esac

if test "$(uname -o 2>/dev/null)" = Android && havecmd termux-clipboard-get; then
    # This is in the "termux-api" package (pkg install termux-api)
    # See also: https://wiki.termux.com/wiki/Termux:API
    exec termux-clipboard-get
fi

if test "$WAYLAND_DISPLAY" && havecmd wl-paste; then
    exec wl-paste --no-newline
fi

if test "$DISPLAY"; then
    if havecmd xsel; then
        exec xsel -ob
    elif havecmd xclip; then
        exec xclip -out -selection clipboard
    fi
fi

echo "Error: no usable clipboard paste command" >&2
exit 1
