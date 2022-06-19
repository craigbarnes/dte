#!/usr/bin/bash

_dte() {
    local dte="$1"
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
        local rcnames="$($dte -B)"
        COMPREPLY=($(compgen -W "$rcnames" -- "$cur"))
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

    compopt -o bashdefault -o default
}

complete -F _dte dte
