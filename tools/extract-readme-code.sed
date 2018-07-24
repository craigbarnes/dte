#!/usr/bin/sed -nf

# This script extracts the installation instructions from README.md as a
# sequence of shell commands. These commands are tested via CI for every
# new commit to ensure they stay valid.

/^Installation$/, /^Documentation$/ {
    /^    ./ {
        s/^    //
        s|^curl .*/dte-[0-9.]\+.tar.gz$|make dist|
        s/sudo *//g
        p
    }
}
