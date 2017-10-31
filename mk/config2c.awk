#!/usr/bin/awk -f

function escape_ident(s) {
    gsub(/[+\/-]/, "_", s)
    return s
}

function escape_string(s) {
    gsub(/\\/, "\\134", s)
    gsub(/"/, "\\042", s)
    gsub(/^ +/, "", s)
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

{
    print "\"" escape_string($0) "\\n\""
}

END {
    print ";\n"
    print "static const struct {"
    print "    const char *const name;"
    print "    const char *const source;"
    print "} builtin_configs[" nfiles "] = {"
    for (i = 1; i <= nfiles; i++) {
        print "    {\"" names[i]  "\", " idents[i] "},"
    }
    print "};"
}
