#!/bin/sh

# This is the script used to generate the list of HTML5 entities
# in config/syntax/html.

curl https://html.spec.whatwg.org/entities.json |
    sed -n 's/^ *"&\([A-Za-z0-9]\+\);\?".*/\1/p' |
    sort |
    uniq |
    tr '\n' ' ' |
    fold -sw68 |
    sed 's/ *$/ \\/' |
    sed '$s/ \\/\n/' |
    sed 's/^/    /'
