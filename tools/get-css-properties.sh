#!/bin/sh

# This is the script used to generate the list of CSS3 properties
# in config/syntax/css.

curl https://www.w3.org/Style/CSS/all-properties.en.json |
    jq -S -r '.[] | .property' |
    uniq |
    tr '\n' ' ' |
    fold -sw70 |
    sed 's/ *$/ \\/' |
    sed '$s/ \\/\n/' |
    sed 's/^/    /'
