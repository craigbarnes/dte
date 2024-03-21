#!/bin/sh
set -eu

# This is the script used to generate the list of CSS properties
# in config/syntax/css

filter='
    [
        .[]
        | select(.status | test("^REC|CR|WD$"))
        | select(.property != "--*")
    ]
    | unique_by(.property)
    | .[].property
'

curl https://www.w3.org/Style/CSS/all-properties.en.json |
    jq -r "$filter" |
    tr '\n' ' ' |
    fold -sw68 |
    sed 's/ *$/ \\/' |
    sed '$s/ \\$/\n/' |
    sed 's/^/    /'
