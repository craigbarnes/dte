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
    {"dart", DART},
    {"dash", SH},
    {"ecl", LISP},
    {"elixir", ELIXIR},
    {"escript", ERLANG},
    {"gawk", AWK},
    {"gjs", JAVASCRIPT},
    {"gmake", MAKE},
    {"gnuplot", GNUPLOT},
    {"groovy", GROOVY},
    {"gsed", SED},
    {"guile", SCHEME},
    {"jruby", RUBY},
    {"julia", JULIA},
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
    {"ocaml", OCAML},
    {"pdksh", SH},
    {"perl", PERL},
    {"php", PHP},
    {"pwsh", POWERSHELL},
    {"python", PYTHON},
    {"r6rs", SCHEME},
    {"racket", SCHEME},
    {"rake", RUBY},
    {"ruby", RUBY},
    {"runghc", HASKELL},
    {"sbcl", LISP},
    {"scala", SCALA},
    {"scheme", SCHEME},
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
