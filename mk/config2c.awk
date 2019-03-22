#!/usr/bin/awk -f

function escape_ident(s) {
    gsub(/[+\/.-]/, "_", s)
    return s
}

function escape_string(s) {
    gsub(/\\/, "\\134", s)
    gsub(/"/, "\\042", s)
    return s
}

function escape_syntax(s) {
    gsub(/^ +/, "", s)
    gsub(/\\/, "\\134", s)
    gsub(/"/, "\\042", s)
    return s
}

BEGIN {
    print "#ifdef __linux__"
    print "#define CONFIG_SECTION SECTION(\".dte.config\") ALIGNED(8)"
    print "#else"
    print "#define CONFIG_SECTION"
    print "#endif\n"
    print "IGNORE_WARNING(\"-Woverlength-strings\")\n"
}

FNR == 1 {
    if (NR != 1) {
        print ";\n"
    }
    name = FILENAME
    gsub(/^config\//, "", name)
    ident = "builtin_" escape_ident(name)
    print "static CONFIG_SECTION const char " ident "[] ="
    names[++nfiles] = name
    idents[nfiles] = ident
}

name ~ /syntax\// {
    if ($0 !~ /^ *#/) {
        print "\"" escape_syntax($0) "\\n\""
    }
    next
}

{
    print "\"" escape_string($0) "\\n\""
}

END {
    print ";\n\nUNIGNORE_WARNINGS\n"

    print \
        "#define cfg(n, t) { \\\n" \
        "    .name = n, \\\n" \
        "    .text = {.data = t, .length = sizeof(t) - 1} \\\n" \
        "}\n"

    print "static CONFIG_SECTION const BuiltinConfig builtin_configs[] = {"
    for (i = 1; i <= nfiles; i++) {
        print "    cfg(\"" names[i] "\", " idents[i] "),"
    }
    print "};"
}
