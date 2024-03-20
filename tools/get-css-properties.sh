#!/bin/sh
set -eu

# This is the script used to generate the list of CSS properties
# in config/syntax/css

curl https://www.w3.org/Style/CSS/all-properties.en.json |
    jq -r '
        [
            .[]
            | select(.status | test("^REC|CR|WD$"))
            | select(.property != "--*")
        ]
        | unique_by(.property)
        | .[].property
    ' |
    tr '\n' ' ' |
    fold -sw68 |
    sed 's/ *$/ \\/' |
    sed '$s/ \\$/\n/' |
    sed 's/^/    /'
