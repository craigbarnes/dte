#!/usr/bin/awk -f

function escape_ident(s) {
    gsub(/[+\/-]/, "_", s)
    return s
}

function escape_string(s) {
    gsub(/^ +/, "", s)
    gsub(/\\/, "\\134", s)
    gsub(/"/, "\\042", s)
    return s
}

FNR == 1 {
    if (NR != 1) {
        print ";\n"
    }
    name = FILENAME
    gsub(/^config\//, "", name)
    ident = "builtin_" escape_ident(name)
    print "static const char " ident "[] ="
    names[++nfiles] = name
    idents[nfiles] = ident
}

/^# *(TODO|FIXME)/ {
    print "\"\\n\""
    next
}

{
    print "\"" escape_string($0) "\\n\""
}

END {
    print ";\n"
    print "static const BuiltinConfig builtin_configs[" nfiles "] = {"
    for (i = 1; i <= nfiles; i++) {
        ident = idents[i]
        print "    {\"" names[i] "\", " ident ", ARRAY_COUNT(" ident ") - 1},"
    }
    print "};"
}
