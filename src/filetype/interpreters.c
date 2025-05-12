static const struct FileInterpreterMap {
    const char key[8];
    const FileTypeEnum filetype;
} interpreters[] = {
    {"ash", SH},
    {"awk", AWK},
    {"bash", SH},
    {"bats", SH}, // https://github.com/bats-core/bats-core
    {"bigloo", SCHEME},
    {"bun", TYPESCRIPT},
    {"ccl", LISP},
    {"chicken", SCHEME},
    {"clisp", LISP},
    {"coffee", COFFEESCRIPT},
    {"crystal", RUBY},
    {"csh", CSH},
    {"dart", DART},
    {"dash", SH},
    {"deno", TYPESCRIPT},
    {"ecl", LISP},
    {"elixir", ELIXIR},
    {"escript", ERLANG},
    {"fish", FISH},
    {"gawk", AWK},
    {"gjs", JAVASCRIPT},
    {"gmake", MAKE},
    {"gnuplot", GNUPLOT},
    {"gojq", JQ}, // https://github.com/itchyny/gojq
    {"groovy", GROOVY},
    {"gsed", SED},
    {"guile", SCHEME},
    {"jaq", JQ}, // https://github.com/01mf02/jaq
    {"jq", JQ},
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
    {"nft", NFTABLES},
    {"node", JAVASCRIPT},
    {"nodejs", JAVASCRIPT},
    {"ocaml", OCAML},
    {"pdksh", SH},
    {"perl", PERL},
    {"php", PHP},
    {"pwsh", POWERSHELL},
    {"pypy", PYTHON},
    {"python", PYTHON},
    {"qjs", JAVASCRIPT},
    {"r6rs", SCHEME},
    {"racket", SCHEME},
    {"rake", RUBY},
    {"rbx", RUBY},
    {"ruby", RUBY},
    {"runghc", HASKELL},
    {"sbcl", LISP},
    {"scala", SCALA},
    {"scheme", SCHEME},
    {"sed", SED},
    {"sh", SH},
    {"tcc", C},
    {"tclsh", TCL},
    {"tcsh", CSH},
    {"ts-node", TYPESCRIPT},
    {"wish", TCL},
    {"zsh", SH},
};

static FileTypeEnum filetype_from_interpreter(const StringView name)
{
    if (name.length < 2) {
        return NONE;
    }

    if (name.length >= ARRAYLEN(interpreters[0].key)) {
        if (strview_equal_cstring(&name, "openrc-run")) {
            return SH;
        } else if (strview_equal_cstring(&name, "runhaskell")) {
            return HASKELL;
        } else if (strview_equal_cstring(&name, "rust-script")) {
            return RUST;
        }
        return NONE;
    }

    const struct FileInterpreterMap *e = BSEARCH(&name, interpreters, ft_compare);
    return e ? e->filetype : NONE;
}
