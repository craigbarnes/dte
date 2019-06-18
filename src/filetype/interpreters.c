static const struct FileInterpreterMap {
    const char key[8];
    const FileTypeEnum filetype;
} interpreters[] = {
    {"ash", SHELL},
    {"awk", AWK},
    {"bash", SHELL},
    {"bigloo", SCHEME},
    {"ccl", LISP},
    {"chicken", SCHEME},
    {"clisp", LISP},
    {"coffee", COFFEESCRIPT},
    {"crystal", RUBY},
    {"dash", SHELL},
    {"ecl", LISP},
    {"gawk", AWK},
    {"gmake", MAKE},
    {"gnuplot", GNUPLOT},
    {"groovy", GROOVY},
    {"gsed", SED},
    {"guile", SCHEME},
    {"jruby", RUBY},
    {"ksh", SHELL},
    {"lisp", LISP},
    {"lua", LUA},
    {"luajit", LUA},
    {"macruby", RUBY},
    {"make", MAKE},
    {"mawk", AWK},
    {"mksh", SHELL},
    {"moon", MOONSCRIPT},
    {"nawk", AWK},
    {"node", JAVASCRIPT},
    {"pdksh", SHELL},
    {"perl", PERL},
    {"php", PHP},
    {"python", PYTHON},
    {"r6rs", SCHEME},
    {"racket", SCHEME},
    {"rake", RUBY},
    {"ruby", RUBY},
    {"sbcl", LISP},
    {"sed", SED},
    {"sh", SHELL},
    {"tcc", C},
    {"tclsh", TCL},
    {"wish", TCL},
    {"zsh", SHELL},
};

static FileTypeEnum filetype_from_interpreter(const StringView sv)
{
    switch (sv.length) {
    case 2: case 3: case 4:
    case 5: case 6: case 7:
        break;
    case 10:
        switch (sv.data[0]) {
        case 'o': return memcmp(sv.data, "openrc-run", 10) ? NONE : SHELL;
        case 'r': return memcmp(sv.data, "runhaskell", 10) ? NONE : HASKELL;
        }
        // Fallthrough
    default:
        return NONE;
    }

    const struct FileInterpreterMap *e = bsearch (
        &sv,
        interpreters,
        ARRAY_COUNT(interpreters),
        sizeof(interpreters[0]),
        ft_compare
    );
    return e ? e->filetype : NONE;
}
