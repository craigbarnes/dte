#!/bin/sh
set -eu

# This is the script used to generate the list of CSS properties
# in config/syntax/css

# Extract names of properties with status REC/CR/WD (without duplicates)
jq_filter='
    [
        .[]
        | select(.status | test("^REC|CR|WD$"))
        | select(.property != "--*")
    ]
    | unique_by(.property)
    | .[].property
'

# Insert indents, line continuations and a final newline
sed_filter='
    s/^/    /
    $ {
        s/$/\n/
        n
    }
    s/ *$/ \\/
'

curl https://www.w3.org/Style/CSS/all-properties.en.json |
    jq -r "$jq_filter" |
    tr '\n' ' ' |
    fold -sw68 |
    sed "$sed_filter"
