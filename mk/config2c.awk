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

function check_busybox_bug() {
    if (escape_syntax("abc\\xyz") == "abc\\134xyz") {
        return
    }
    # A bug in BusyBox 1.37.0 AWK breaks the escape_*() functions above,
    # so we explicitly detect it and show a clear error message, instead
    # of producing broken output
    ref = "       See: https://gitlab.com/craigbarnes/dte/-/issues/226"
    print "ERROR: AWK interpreter doesn't conform to POSIX\n" ref > "/dev/stderr"
    exit(1)
}

BEGIN {
    check_busybox_bug()
    print "IGNORE_WARNING(\"-Woverlength-strings\")\n"
    nfiles = 0
}

FNR == 1 {
    if (NR != 1) {
        print ";\n"
    }
    name = FILENAME
    gsub(/^config\//, "", name)
    ident = "builtin_" escape_ident(name)
    print "CONFIG_SECTION static const char " ident "[] ="
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
    print "CONFIG_SECTION static const BuiltinConfig builtin_configs[] = {"

    for (i = 1; i <= nfiles; i++) {
        print "    CFG(\"" names[i] "\", " idents[i] "),"
    }

    print "};"
}
