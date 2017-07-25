#!/usr/bin/sed -f

# Append C statement terminator to the end
$a\;

# Delete comments and blank lines
/^#/d
/^$/d

# Escape backslashes and double quotes
s/\\/\\\\/g
s/"/\\"/g

# Wrap each line in double quotes
s/^/"/
s/$/\\n"/
