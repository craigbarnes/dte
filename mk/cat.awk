#!/usr/bin/awk -f

# Print the contents of all input files, with clear separators between them.
# This can be used to `cat(1)` build system generated files and log files in
# a way that's more convenient for debugging.

FNR == 1 {
    print "\n----------------------------------------------------------------------------"
    print FILENAME ":"
    print "----------------------------------------------------------------------------\n"
}

{
    print
}

END {
    print "\n----------------------------------------------------------------------------\n"
}
