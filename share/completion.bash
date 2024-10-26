#!/usr/bin/bash

# shellcheck disable=SC2207
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
        COMPREPLY=($(compgen -W "$($dte -B)" -- "$cur"))
        return;;
    -t)
        local format="(if (prefix? \$name \"$cur\") (list \$name #t) #f)"
        COMPREPLY=($(readtags -F "$format" -l 2>/dev/null))
        return;;
    -[cBhKV])
        return;;
    esac

    compopt -o bashdefault -o default
}

complete -F _dte dte
