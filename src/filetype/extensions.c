static const struct FileExtensionMap {
    const char ext[12];
    const FileTypeEnum filetype;
} extensions[] = {
    {"ada", ADA},
    {"adb", ADA},
    {"ads", ADA},
    {"asd", LISP},
    {"asm", ASSEMBLY},
    {"auk", AWK},
    {"automount", INI},
    {"awk", AWK},
    {"bash", SHELL},
    {"bat", BATCHFILE},
    {"bbl", TEX},
    {"bib", BIBTEX},
    {"btm", BATCHFILE},
    {"c++", CPLUSPLUS},
    {"cc", CPLUSPLUS},
    {"cl", LISP},
    {"clj", CLOJURE},
    {"cls", TEX},
    {"cmake", CMAKE},
    {"cmd", BATCHFILE},
    {"coffee", COFFEESCRIPT},
    {"cpp", CPLUSPLUS},
    {"cr", RUBY},
    {"cs", CSHARP},
    {"cson", COFFEESCRIPT},
    {"css", CSS},
    {"csv", CSV},
    {"cxx", CPLUSPLUS},
    {"dart", DART},
    {"desktop", INI},
    {"di", D},
    {"diff", DIFF},
    {"doap", XML},
    {"docbook", XML},
    {"docker", DOCKER},
    {"dot", DOT},
    {"doxy", CONFIG},
    {"dterc", DTERC},
    {"dtx", TEX},
    {"ebuild", SHELL},
    {"el", LISP},
    {"emacs", EMACSLISP},
    {"eml", MAIL},
    {"eps", POSTSCRIPT},
    {"flatpakref", INI},
    {"flatpakrepo", INI},
    {"frag", GLSL},
    {"gawk", AWK},
    {"gemspec", RUBY},
    {"geojson", JSON},
    {"glsl", GLSL},
    {"glslf", GLSL},
    {"glslv", GLSL},
    {"gnuplot", GNUPLOT},
    {"go", GO},
    {"gp", GNUPLOT},
    {"gperf", GPERF},
    {"gpi", GNUPLOT},
    {"groovy", GROOVY},
    {"gsed", SED},
    {"gv", DOT},
    {"hh", CPLUSPLUS},
    {"hpp", CPLUSPLUS},
    {"hs", HASKELL},
    {"htm", HTML},
    {"html", HTML},
    {"hxx", CPLUSPLUS},
    {"ini", INI},
    {"ins", TEX},
    {"java", JAVA},
    {"js", JAVASCRIPT},
    {"json", JSON},
    {"ksh", SHELL},
    {"lsp", LISP},
    {"ltx", TEX},
    {"lua", LUA},
    {"m4", M4},
    {"mak", MAKE},
    {"make", MAKE},
    {"markdown", MARKDOWN},
    {"mawk", AWK},
    {"md", MARKDOWN},
    {"mk", MAKE},
    {"mkd", MARKDOWN},
    {"mkdn", MARKDOWN},
    {"moon", MOONSCRIPT},
    {"mount", INI},
    {"nawk", AWK},
    {"nginx", NGINX},
    {"nginxconf", NGINX},
    {"nim", NIM},
    {"ninja", NINJA},
    {"nix", NIX},
    {"page", XML},
    {"patch", DIFF},
    {"path", INI},
    {"pc", PKGCONFIG},
    {"perl", PERL},
    {"php", PHP},
    {"pl", PERL},
    {"pls", INI},
    {"plt", GNUPLOT},
    {"pm", PERL},
    {"po", GETTEXT},
    {"pot", GETTEXT},
    {"pp", RUBY},
    {"proto", PROTOBUF},
    {"ps", POSTSCRIPT},
    {"py", PYTHON},
    {"py3", PYTHON},
    {"rake", RUBY},
    {"rb", RUBY},
    {"rdf", XML},
    {"rkt", RACKET},
    {"rktd", RACKET},
    {"rktl", RACKET},
    {"rockspec", LUA},
    {"rs", RUST},
    {"rst", RESTRUCTUREDTEXT},
    {"scala", SCALA},
    {"scm", SCHEME},
    {"scss", SCSS},
    {"sed", SED},
    {"service", INI},
    {"sh", SHELL},
    {"sld", SCHEME},
    {"slice", INI},
    {"sls", SCHEME},
    {"socket", INI},
    {"sql", SQL},
    {"ss", SCHEME},
    {"sty", TEX},
    {"svg", XML},
    {"target", INI},
    {"tcl", TCL},
    {"tex", TEX},
    {"texi", TEXINFO},
    {"texinfo", TEXINFO},
    {"timer", INI},
    {"toml", TOML},
    {"topojson", JSON},
    {"ts", TYPESCRIPT},
    {"tsx", TYPESCRIPT},
    {"ui", XML},
    {"vala", VALA},
    {"vapi", VALA},
    {"vcard", VCARD},
    {"vcf", VCARD},
    {"ver", VERILOG},
    {"vert", GLSL},
    {"vh", VHDL},
    {"vhd", VHDL},
    {"vhdl", VHDL},
    {"vim", VIML},
    {"wsgi", PYTHON},
    {"xhtml", HTML},
    {"xml", XML},
    {"xsd", XML},
    {"xsl", XML},
    {"xslt", XML},
    {"yaml", YAML},
    {"yml", YAML},
    {"zsh", SHELL},
};

static FileTypeEnum filetype_from_extension(const char *s, size_t len)
{
    switch (len) {
    case 1:
        switch (s[0]) {
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
            return ROFF;
        case 'c': case 'h':
            return C;
        case 'C': case 'H':
            return CPLUSPLUS;
        case 'S': case 's':
            return ASSEMBLY;
        case 'd': return D;
        case 'l': return LEX;
        case 'm': return OBJECTIVEC;
        case 'v': return VERILOG;
        case 'y': return YACC;
        }
        return NONE;
    case 2: case 3: case 4: case 5:
    case 6: case 7: case 8: case 9:
    case 10: case 11:
        break;
    default:
        return NONE;
    }

    const StringView sv = string_view(s, len);
    const struct FileExtensionMap *e = bsearch (
        &sv,
        extensions,
        ARRAY_COUNT(extensions),
        sizeof(extensions[0]),
        ft_compare
    );
    return e ? e->filetype : NONE;
}
