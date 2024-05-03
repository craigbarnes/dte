#!/bin/sh

# This is the script used to generate the list of HTML5 entities
# in config/syntax/html

# Extract the subset of entities with no final semicolon, then
# remove leading ampersands
jq_filter='
    keys_unsorted
    | .[]
    | select(endswith(";") | not)
    | sub("^&"; "")
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

curl https://html.spec.whatwg.org/entities.json |
    jq -r "$jq_filter" |
    sort |
    tr '\n' ' ' |
    fold -sw68 |
    sed "$sed_filter"
