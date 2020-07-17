#!/usr/bin/bash

_dte() {
    local cur="$2"
    local prev="$3"

    case "$cur" in
    -)
        COMPREPLY=($(compgen -W "-h -H -K -B -R -V -c -t -r -b -s" -- "$cur"))
        return;;
    -[bcrstHR])
        COMPREPLY=("$cur")
        return;;
    -*) # -[hBKV]
        return;;
    esac

    case "$prev" in
    -b)
        COMPREPLY=($(compgen -W "$(dte -B)" -- "$cur"))
        return;;
    -t)
        COMPREPLY=($(
            readtags -Q "(prefix? \$name \"$cur\")" -l 2>/dev/null |
            cut -f1 |
            head -n50000
        ))
        return;;
    -[cBhKV])
        return;;
    esac

    compopt -o default
}

complete -F _dte dte
