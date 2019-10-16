#!/usr/bin/sed -f

# This script extracts the installation instructions from README.md as a
# sequence of shell commands. These commands are tested via CI for every
# new commit to ensure they stay valid.

/^Installation$/, /^Documentation$/ {
    /^    ./ {
        s/^    //
        s|^curl .*/dte-[0-9.]\+.tar.gz$|make dist-latest-release|
        s/sudo *//g
        p
    }
}

# Suppress default printing (like "sed -n", but more reliable)
d
