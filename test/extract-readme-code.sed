#!/usr/bin/sed -nf

/^Installation$/, /^Documentation$/ {
    /^    ./ {
        s/^    //
        s/sudo *//g
        p
    }
}
