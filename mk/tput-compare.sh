#!/bin/sh
set -e

escaped_tput() {
    tput $* 2>/dev/null | sed 's|\x1b|\\E|g';
    echo
}

if test -z "$terms"; then
    terms='ansi vt220 linux xterm tmux screen'
fi

if test -z "$caps"; then
    caps='
        el rmkx smkx rmcup smcup
        kich1 kdch1 khome kend kpp knp
        kcub1 kcuf1 kcuu1 kcud1
        kf1 kf2 kf3 kf4 kf5 kf6 kf7 kf8
        kLFT kRIT kHOM kEND
    '
fi

col='%-10s'
col1=' %-6s |'

printf "$col1 $col $col $col $col $col $col\n" cap $terms
printf '%78s\n' |tr " " "-"

for cap in $caps; do
    printf "$col1" "$cap"
    for term in $terms; do
        printf " $col" "$(escaped_tput -T $term $cap)"
    done
    printf '\n'
done
