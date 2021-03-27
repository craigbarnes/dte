static const struct FileInterpreterMap {
    const char key[8];
    const FileTypeEnum filetype;
} interpreters[] = {
    {"ash", SH},
    {"awk", AWK},
    {"bash", SH},
    {"bigloo", SCHEME},
    {"ccl", LISP},
    {"chicken", SCHEME},
    {"clisp", LISP},
    {"coffee", COFFEESCRIPT},
    {"crystal", RUBY},
    {"dash", SH},
    {"ecl", LISP},
    {"gawk", AWK},
    {"gmake", MAKE},
    {"gnuplot", GNUPLOT},
    {"groovy", GROOVY},
    {"gsed", SED},
    {"guile", SCHEME},
    {"jruby", RUBY},
    {"ksh", SH},
    {"lisp", LISP},
    {"lua", LUA},
    {"luajit", LUA},
    {"macruby", RUBY},
    {"make", MAKE},
    {"mawk", AWK},
    {"mksh", SH},
    {"moon", MOONSCRIPT},
    {"nawk", AWK},
    {"node", JAVASCRIPT},
    {"pdksh", SH},
    {"perl", PERL},
    {"php", PHP},
    {"python", PYTHON},
    {"r6rs", SCHEME},
    {"racket", SCHEME},
    {"rake", RUBY},
    {"ruby", RUBY},
    {"sbcl", LISP},
    {"sed", SED},
    {"sh", SH},
    {"tcc", C},
    {"tclsh", TCL},
    {"wish", TCL},
    {"zsh", SH},
};

static FileTypeEnum filetype_from_interpreter(const StringView name)
{
    if (name.length < 2) {
        return NONE;
    }

    if (name.length >= ARRAY_COUNT(interpreters[0].key)) {
        if (strview_equal_cstring(&name, "openrc-run")) {
            return SH;
        } else if (strview_equal_cstring(&name, "runhaskell")) {
            return HASKELL;
        }
        return NONE;
    }

    const struct FileInterpreterMap *e = BSEARCH(&name, interpreters, ft_compare);
    return e ? e->filetype : NONE;
}
