#!/usr/bin/bash

_dte() {
    local cur prev words cword split
    _init_completion -s || return

    if test "$cur" = '-'; then
        COMPREPLY=($(compgen -W "-h -B -R -V -c -t -r -b" -- "$cur"))
        return
    fi

    case "$prev" in
    -b)
        COMPREPLY=($(compgen -W "$(dte -B)" -- "$cur"))
        return;;
    -c)
        COMPREPLY=()
        return;;
    -t)
        COMPREPLY=($(dte -T "$cur" | head -n50000))
        return;;
    esac

    COMPREPLY=($(compgen -o default -- "$cur"))
}

complete -F _dte dte
