static const struct {
    const char key[16];
    const FileTypeEnum val;
} filetype_from_extension_table[183] = {
    {"1", ROFF},
    {"2", ROFF},
    {"3", ROFF},
    {"4", ROFF},
    {"5", ROFF},
    {"6", ROFF},
    {"7", ROFF},
    {"8", ROFF},
    {"9", ROFF},
    {"C", CPLUSPLUS},
    {"H", CPLUSPLUS},
    {"S", ASSEMBLY},
    {"ada", ADA},
    {"adb", ADA},
    {"ads", ADA},
    {"asd", COMMONLISP},
    {"asm", ASSEMBLY},
    {"auk", AWK},
    {"automount", INI},
    {"awk", AWK},
    {"bash", SHELL},
    {"bat", BATCHFILE},
    {"bbl", TEX},
    {"bib", BIBTEX},
    {"btm", BATCHFILE},
    {"c", C},
    {"c++", CPLUSPLUS},
    {"cc", CPLUSPLUS},
    {"cl", COMMONLISP},
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
    {"d", D},
    {"dart", DART},
    {"desktop", INI},
    {"di", D},
    {"diff", DIFF},
    {"doap", XML},
    {"docbook", XML},
    {"docker", DOCKER},
    {"dot", DOT},
    {"doxy", DOXYGEN},
    {"dterc", DTERC},
    {"dtx", TEX},
    {"ebuild", SHELL},
    {"el", EMACSLISP},
    {"emacs", EMACSLISP},
    {"eml", EMAIL},
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
    {"h", C},
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
    {"l", LEX},
    {"lsp", COMMONLISP},
    {"ltx", TEX},
    {"lua", LUA},
    {"m", OBJECTIVEC},
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
    {"page", MALLARD},
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
    {"postscript", POSTSCRIPT},
    {"pot", GETTEXT},
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
    {"s", ASSEMBLY},
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
    {"v", VERILOG},
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
    {"y", YACC},
    {"yaml", YAML},
    {"yml", YAML},
    {"zsh", SHELL},
};

static FileTypeEnum filetype_from_extension(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 1:
        switch (s[0]) {
        case '1':
            return ROFF;
        case '2':
            return ROFF;
        case '3':
            return ROFF;
        case '4':
            return ROFF;
        case '5':
            return ROFF;
        case '6':
            return ROFF;
        case '7':
            return ROFF;
        case '8':
            return ROFF;
        case '9':
            return ROFF;
        case 'C':
            return CPLUSPLUS;
        case 'H':
            return CPLUSPLUS;
        case 'S':
            return ASSEMBLY;
        case 'c':
            return C;
        case 'd':
            return D;
        case 'h':
            return C;
        case 'l':
            return LEX;
        case 'm':
            return OBJECTIVEC;
        case 's':
            return ASSEMBLY;
        case 'v':
            return VERILOG;
        case 'y':
            return YACC;
        }
        break;
    case 2:
        switch (s[0]) {
        case 'c':
            switch (s[1]) {
            case 'c':
                return CPLUSPLUS;
            case 'l':
                return COMMONLISP;
            case 'r':
                return RUBY;
            case 's':
                return CSHARP;
            }
            break;
        case 'd':
            idx = 44; // di
            goto compare;
        case 'e':
            idx = 54; // el
            goto compare;
        case 'g':
            switch (s[1]) {
            case 'o':
                return GO;
            case 'p':
                return GNUPLOT;
            case 'v':
                return DOT;
            }
            break;
        case 'h':
            switch (s[1]) {
            case 'h':
                return CPLUSPLUS;
            case 's':
                return HASKELL;
            }
            break;
        case 'j':
            idx = 85; // js
            goto compare;
        case 'm':
            switch (s[1]) {
            case '4':
                return M4;
            case 'd':
                return MARKDOWN;
            case 'k':
                return MAKE;
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'c':
                return PKGCONFIG;
            case 'l':
                return PERL;
            case 'm':
                return PERL;
            case 'o':
                return GETTEXT;
            case 's':
                return POSTSCRIPT;
            case 'y':
                return PYTHON;
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'b':
                return RUBY;
            case 's':
                return RUST;
            }
            break;
        case 's':
            switch (s[1]) {
            case 'h':
                return SHELL;
            case 's':
                return SCHEME;
            }
            break;
        case 't':
            idx = 159; // ts
            goto compare;
        case 'u':
            idx = 161; // ui
            goto compare;
        case 'v':
            idx = 169; // vh
            goto compare;
        }
        break;
    case 3:
        switch (s[0]) {
        case 'a':
            switch (s[1]) {
            case 'd':
                switch (s[2]) {
                case 'a':
                    return ADA;
                case 'b':
                    return ADA;
                case 's':
                    return ADA;
                }
                break;
            case 's':
                switch (s[2]) {
                case 'd':
                    return COMMONLISP;
                case 'm':
                    return ASSEMBLY;
                }
                break;
            case 'u':
                idx = 17; // auk
                goto compare;
            case 'w':
                idx = 19; // awk
                goto compare;
            }
            break;
        case 'b':
            switch (s[1]) {
            case 'a':
                idx = 21; // bat
                goto compare;
            case 'b':
                idx = 22; // bbl
                goto compare;
            case 'i':
                idx = 23; // bib
                goto compare;
            case 't':
                idx = 24; // btm
                goto compare;
            }
            break;
        case 'c':
            switch (s[1]) {
            case '+':
                idx = 26; // c++
                goto compare;
            case 'l':
                switch (s[2]) {
                case 'j':
                    return CLOJURE;
                case 's':
                    return TEX;
                }
                break;
            case 'm':
                idx = 32; // cmd
                goto compare;
            case 'p':
                idx = 34; // cpp
                goto compare;
            case 's':
                switch (s[2]) {
                case 's':
                    return CSS;
                case 'v':
                    return CSV;
                }
                break;
            case 'x':
                idx = 40; // cxx
                goto compare;
            }
            break;
        case 'd':
            switch (s[1]) {
            case 'o':
                idx = 49; // dot
                goto compare;
            case 't':
                idx = 52; // dtx
                goto compare;
            }
            break;
        case 'e':
            switch (s[1]) {
            case 'm':
                idx = 56; // eml
                goto compare;
            case 'p':
                idx = 57; // eps
                goto compare;
            }
            break;
        case 'g':
            idx = 71; // gpi
            goto compare;
        case 'h':
            switch (s[1]) {
            case 'p':
                idx = 77; // hpp
                goto compare;
            case 't':
                idx = 79; // htm
                goto compare;
            case 'x':
                idx = 81; // hxx
                goto compare;
            }
            break;
        case 'i':
            switch (s[1]) {
            case 'n':
                switch (s[2]) {
                case 'i':
                    return INI;
                case 's':
                    return TEX;
                }
                break;
            }
            break;
        case 'k':
            idx = 87; // ksh
            goto compare;
        case 'l':
            switch (s[1]) {
            case 's':
                idx = 89; // lsp
                goto compare;
            case 't':
                idx = 90; // ltx
                goto compare;
            case 'u':
                idx = 91; // lua
                goto compare;
            }
            break;
        case 'm':
            switch (s[1]) {
            case 'a':
                idx = 94; // mak
                goto compare;
            case 'k':
                idx = 100; // mkd
                goto compare;
            }
            break;
        case 'n':
            switch (s[1]) {
            case 'i':
                switch (s[2]) {
                case 'm':
                    return NIM;
                case 'x':
                    return NIX;
                }
                break;
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'h':
                idx = 115; // php
                goto compare;
            case 'l':
                switch (s[2]) {
                case 's':
                    return INI;
                case 't':
                    return GNUPLOT;
                }
                break;
            case 'o':
                idx = 122; // pot
                goto compare;
            case 'y':
                idx = 126; // py3
                goto compare;
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'd':
                idx = 129; // rdf
                goto compare;
            case 'k':
                idx = 130; // rkt
                goto compare;
            case 's':
                idx = 135; // rst
                goto compare;
            }
            break;
        case 's':
            switch (s[1]) {
            case 'c':
                idx = 138; // scm
                goto compare;
            case 'e':
                idx = 140; // sed
                goto compare;
            case 'l':
                switch (s[2]) {
                case 'd':
                    return SCHEME;
                case 's':
                    return SCHEME;
                }
                break;
            case 'q':
                idx = 147; // sql
                goto compare;
            case 't':
                idx = 149; // sty
                goto compare;
            case 'v':
                idx = 150; // svg
                goto compare;
            }
            break;
        case 't':
            switch (s[1]) {
            case 'c':
                idx = 152; // tcl
                goto compare;
            case 'e':
                idx = 153; // tex
                goto compare;
            case 's':
                idx = 160; // tsx
                goto compare;
            }
            break;
        case 'v':
            switch (s[1]) {
            case 'c':
                idx = 166; // vcf
                goto compare;
            case 'e':
                idx = 167; // ver
                goto compare;
            case 'h':
                idx = 170; // vhd
                goto compare;
            case 'i':
                idx = 172; // vim
                goto compare;
            }
            break;
        case 'x':
            switch (s[1]) {
            case 'm':
                idx = 175; // xml
                goto compare;
            case 's':
                switch (s[2]) {
                case 'd':
                    return XML;
                case 'l':
                    return XML;
                }
                break;
            }
            break;
        case 'y':
            idx = 181; // yml
            goto compare;
        case 'z':
            idx = 182; // zsh
            goto compare;
        }
        break;
    case 4:
        switch (s[0]) {
        case 'b':
            idx = 20; // bash
            goto compare;
        case 'c':
            idx = 37; // cson
            goto compare;
        case 'd':
            switch (s[1]) {
            case 'a':
                idx = 42; // dart
                goto compare;
            case 'i':
                idx = 45; // diff
                goto compare;
            case 'o':
                switch (s[2]) {
                case 'a':
                    idx = 46; // doap
                    goto compare;
                case 'x':
                    idx = 50; // doxy
                    goto compare;
                }
                break;
            }
            break;
        case 'f':
            idx = 60; // frag
            goto compare;
        case 'g':
            switch (s[1]) {
            case 'a':
                idx = 61; // gawk
                goto compare;
            case 'l':
                idx = 64; // glsl
                goto compare;
            case 's':
                idx = 73; // gsed
                goto compare;
            }
            break;
        case 'h':
            idx = 80; // html
            goto compare;
        case 'j':
            switch (s[1]) {
            case 'a':
                idx = 84; // java
                goto compare;
            case 's':
                idx = 86; // json
                goto compare;
            }
            break;
        case 'm':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'k':
                    idx = 95; // make
                    goto compare;
                case 'w':
                    idx = 97; // mawk
                    goto compare;
                }
                break;
            case 'k':
                idx = 101; // mkdn
                goto compare;
            case 'o':
                idx = 102; // moon
                goto compare;
            }
            break;
        case 'n':
            idx = 104; // nawk
            goto compare;
        case 'p':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'g':
                    idx = 110; // page
                    goto compare;
                case 't':
                    idx = 112; // path
                    goto compare;
                }
                break;
            case 'e':
                idx = 114; // perl
                goto compare;
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'a':
                idx = 127; // rake
                goto compare;
            case 'k':
                switch (s[2]) {
                case 't':
                    switch (s[3]) {
                    case 'd':
                        return RACKET;
                    case 'l':
                        return RACKET;
                    }
                    break;
                }
                break;
            }
            break;
        case 's':
            idx = 139; // scss
            goto compare;
        case 't':
            switch (s[1]) {
            case 'e':
                idx = 154; // texi
                goto compare;
            case 'o':
                idx = 157; // toml
                goto compare;
            }
            break;
        case 'v':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'l':
                    idx = 163; // vala
                    goto compare;
                case 'p':
                    idx = 164; // vapi
                    goto compare;
                }
                break;
            case 'e':
                idx = 168; // vert
                goto compare;
            case 'h':
                idx = 171; // vhdl
                goto compare;
            }
            break;
        case 'w':
            idx = 173; // wsgi
            goto compare;
        case 'x':
            idx = 178; // xslt
            goto compare;
        case 'y':
            idx = 180; // yaml
            goto compare;
        }
        break;
    case 5:
        switch (s[0]) {
        case 'c':
            idx = 31; // cmake
            goto compare;
        case 'd':
            idx = 51; // dterc
            goto compare;
        case 'e':
            idx = 55; // emacs
            goto compare;
        case 'g':
            switch (s[1]) {
            case 'l':
                switch (s[4]) {
                case 'f':
                    return GLSL;
                case 'v':
                    return GLSL;
                }
                break;
            case 'p':
                idx = 70; // gperf
                goto compare;
            }
            break;
        case 'm':
            idx = 103; // mount
            goto compare;
        case 'n':
            switch (s[1]) {
            case 'g':
                idx = 105; // nginx
                goto compare;
            case 'i':
                idx = 108; // ninja
                goto compare;
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'a':
                idx = 111; // patch
                goto compare;
            case 'r':
                idx = 123; // proto
                goto compare;
            }
            break;
        case 's':
            switch (s[1]) {
            case 'c':
                idx = 137; // scala
                goto compare;
            case 'l':
                idx = 144; // slice
                goto compare;
            }
            break;
        case 't':
            idx = 156; // timer
            goto compare;
        case 'v':
            idx = 165; // vcard
            goto compare;
        case 'x':
            idx = 174; // xhtml
            goto compare;
        }
        break;
    case 6:
        switch (s[0]) {
        case 'c':
            idx = 33; // coffee
            goto compare;
        case 'd':
            idx = 48; // docker
            goto compare;
        case 'e':
            idx = 53; // ebuild
            goto compare;
        case 'g':
            idx = 72; // groovy
            goto compare;
        case 's':
            idx = 146; // socket
            goto compare;
        case 't':
            idx = 151; // target
            goto compare;
        }
        break;
    case 7:
        switch (s[0]) {
        case 'd':
            switch (s[1]) {
            case 'e':
                idx = 43; // desktop
                goto compare;
            case 'o':
                idx = 47; // docbook
                goto compare;
            }
            break;
        case 'g':
            switch (s[1]) {
            case 'e':
                switch (s[2]) {
                case 'm':
                    idx = 62; // gemspec
                    goto compare;
                case 'o':
                    idx = 63; // geojson
                    goto compare;
                }
                break;
            case 'n':
                idx = 67; // gnuplot
                goto compare;
            }
            break;
        case 's':
            idx = 141; // service
            goto compare;
        case 't':
            idx = 155; // texinfo
            goto compare;
        }
        break;
    case 8:
        switch (s[0]) {
        case 'm':
            idx = 96; // markdown
            goto compare;
        case 'r':
            idx = 133; // rockspec
            goto compare;
        case 't':
            idx = 158; // topojson
            goto compare;
        }
        break;
    case 9:
        switch (s[0]) {
        case 'a':
            idx = 18; // automount
            goto compare;
        case 'n':
            idx = 106; // nginxconf
            goto compare;
        }
        break;
    case 10:
        switch (s[0]) {
        case 'f':
            idx = 58; // flatpakref
            goto compare;
        case 'p':
            idx = 121; // postscript
            goto compare;
        }
        break;
    case 11:
        idx = 59; // flatpakrepo
        goto compare;
    }
    return 0;
compare:
    key = filetype_from_extension_table[idx].key;
    val = filetype_from_extension_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}
