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

#define CMP(i) idx = i; goto compare

static FileTypeEnum filetype_from_extension(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 1:
        switch (s[0]) {
        case '1': return ROFF;
        case '2': return ROFF;
        case '3': return ROFF;
        case '4': return ROFF;
        case '5': return ROFF;
        case '6': return ROFF;
        case '7': return ROFF;
        case '8': return ROFF;
        case '9': return ROFF;
        case 'C': return CPLUSPLUS;
        case 'H': return CPLUSPLUS;
        case 'S': return ASSEMBLY;
        case 'c': return C;
        case 'd': return D;
        case 'h': return C;
        case 'l': return LEX;
        case 'm': return OBJECTIVEC;
        case 's': return ASSEMBLY;
        case 'v': return VERILOG;
        case 'y': return YACC;
        }
        break;
    case 2:
        switch (s[0]) {
        case 'c':
            switch (s[1]) {
            case 'c': return CPLUSPLUS;
            case 'l': return COMMONLISP;
            case 'r': return RUBY;
            case 's': return CSHARP;
            }
            break;
        case 'd': CMP(44); // di
        case 'e': CMP(54); // el
        case 'g':
            switch (s[1]) {
            case 'o': return GO;
            case 'p': return GNUPLOT;
            case 'v': return DOT;
            }
            break;
        case 'h':
            switch (s[1]) {
            case 'h': return CPLUSPLUS;
            case 's': return HASKELL;
            }
            break;
        case 'j': CMP(85); // js
        case 'm':
            switch (s[1]) {
            case '4': return M4;
            case 'd': return MARKDOWN;
            case 'k': return MAKE;
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'c': return PKGCONFIG;
            case 'l': return PERL;
            case 'm': return PERL;
            case 'o': return GETTEXT;
            case 's': return POSTSCRIPT;
            case 'y': return PYTHON;
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'b': return RUBY;
            case 's': return RUST;
            }
            break;
        case 's':
            switch (s[1]) {
            case 'h': return SHELL;
            case 's': return SCHEME;
            }
            break;
        case 't': CMP(159); // ts
        case 'u': CMP(161); // ui
        case 'v': CMP(169); // vh
        }
        break;
    case 3:
        switch (s[0]) {
        case 'a':
            switch (s[1]) {
            case 'd':
                switch (s[2]) {
                case 'a': return ADA;
                case 'b': return ADA;
                case 's': return ADA;
                }
                break;
            case 's':
                switch (s[2]) {
                case 'd': return COMMONLISP;
                case 'm': return ASSEMBLY;
                }
                break;
            case 'u': CMP(17); // auk
            case 'w': CMP(19); // awk
            }
            break;
        case 'b':
            switch (s[1]) {
            case 'a': CMP(21); // bat
            case 'b': CMP(22); // bbl
            case 'i': CMP(23); // bib
            case 't': CMP(24); // btm
            }
            break;
        case 'c':
            switch (s[1]) {
            case '+': CMP(26); // c++
            case 'l':
                switch (s[2]) {
                case 'j': return CLOJURE;
                case 's': return TEX;
                }
                break;
            case 'm': CMP(32); // cmd
            case 'p': CMP(34); // cpp
            case 's':
                switch (s[2]) {
                case 's': return CSS;
                case 'v': return CSV;
                }
                break;
            case 'x': CMP(40); // cxx
            }
            break;
        case 'd':
            switch (s[1]) {
            case 'o': CMP(49); // dot
            case 't': CMP(52); // dtx
            }
            break;
        case 'e':
            switch (s[1]) {
            case 'm': CMP(56); // eml
            case 'p': CMP(57); // eps
            }
            break;
        case 'g': CMP(71); // gpi
        case 'h':
            switch (s[1]) {
            case 'p': CMP(77); // hpp
            case 't': CMP(79); // htm
            case 'x': CMP(81); // hxx
            }
            break;
        case 'i':
            switch (s[1]) {
            case 'n':
                switch (s[2]) {
                case 'i': return INI;
                case 's': return TEX;
                }
                break;
            }
            break;
        case 'k': CMP(87); // ksh
        case 'l':
            switch (s[1]) {
            case 's': CMP(89); // lsp
            case 't': CMP(90); // ltx
            case 'u': CMP(91); // lua
            }
            break;
        case 'm':
            switch (s[1]) {
            case 'a': CMP(94); // mak
            case 'k': CMP(100); // mkd
            }
            break;
        case 'n':
            switch (s[1]) {
            case 'i':
                switch (s[2]) {
                case 'm': return NIM;
                case 'x': return NIX;
                }
                break;
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'h': CMP(115); // php
            case 'l':
                switch (s[2]) {
                case 's': return INI;
                case 't': return GNUPLOT;
                }
                break;
            case 'o': CMP(122); // pot
            case 'y': CMP(126); // py3
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'd': CMP(129); // rdf
            case 'k': CMP(130); // rkt
            case 's': CMP(135); // rst
            }
            break;
        case 's':
            switch (s[1]) {
            case 'c': CMP(138); // scm
            case 'e': CMP(140); // sed
            case 'l':
                switch (s[2]) {
                case 'd': return SCHEME;
                case 's': return SCHEME;
                }
                break;
            case 'q': CMP(147); // sql
            case 't': CMP(149); // sty
            case 'v': CMP(150); // svg
            }
            break;
        case 't':
            switch (s[1]) {
            case 'c': CMP(152); // tcl
            case 'e': CMP(153); // tex
            case 's': CMP(160); // tsx
            }
            break;
        case 'v':
            switch (s[1]) {
            case 'c': CMP(166); // vcf
            case 'e': CMP(167); // ver
            case 'h': CMP(170); // vhd
            case 'i': CMP(172); // vim
            }
            break;
        case 'x':
            switch (s[1]) {
            case 'm': CMP(175); // xml
            case 's':
                switch (s[2]) {
                case 'd': return XML;
                case 'l': return XML;
                }
                break;
            }
            break;
        case 'y': CMP(181); // yml
        case 'z': CMP(182); // zsh
        }
        break;
    case 4:
        switch (s[0]) {
        case 'b': CMP(20); // bash
        case 'c': CMP(37); // cson
        case 'd':
            switch (s[1]) {
            case 'a': CMP(42); // dart
            case 'i': CMP(45); // diff
            case 'o':
                switch (s[2]) {
                case 'a': CMP(46); // doap
                case 'x': CMP(50); // doxy
                }
                break;
            }
            break;
        case 'f': CMP(60); // frag
        case 'g':
            switch (s[1]) {
            case 'a': CMP(61); // gawk
            case 'l': CMP(64); // glsl
            case 's': CMP(73); // gsed
            }
            break;
        case 'h': CMP(80); // html
        case 'j':
            switch (s[1]) {
            case 'a': CMP(84); // java
            case 's': CMP(86); // json
            }
            break;
        case 'm':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'k': CMP(95); // make
                case 'w': CMP(97); // mawk
                }
                break;
            case 'k': CMP(101); // mkdn
            case 'o': CMP(102); // moon
            }
            break;
        case 'n': CMP(104); // nawk
        case 'p':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'g': CMP(110); // page
                case 't': CMP(112); // path
                }
                break;
            case 'e': CMP(114); // perl
            }
            break;
        case 'r':
            switch (s[1]) {
            case 'a': CMP(127); // rake
            case 'k':
                switch (s[2]) {
                case 't':
                    switch (s[3]) {
                    case 'd': return RACKET;
                    case 'l': return RACKET;
                    }
                    break;
                }
                break;
            }
            break;
        case 's': CMP(139); // scss
        case 't':
            switch (s[1]) {
            case 'e': CMP(154); // texi
            case 'o': CMP(157); // toml
            }
            break;
        case 'v':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'l': CMP(163); // vala
                case 'p': CMP(164); // vapi
                }
                break;
            case 'e': CMP(168); // vert
            case 'h': CMP(171); // vhdl
            }
            break;
        case 'w': CMP(173); // wsgi
        case 'x': CMP(178); // xslt
        case 'y': CMP(180); // yaml
        }
        break;
    case 5:
        switch (s[0]) {
        case 'c': CMP(31); // cmake
        case 'd': CMP(51); // dterc
        case 'e': CMP(55); // emacs
        case 'g':
            switch (s[1]) {
            case 'l':
                switch (s[4]) {
                case 'f': return GLSL;
                case 'v': return GLSL;
                }
                break;
            case 'p': CMP(70); // gperf
            }
            break;
        case 'm': CMP(103); // mount
        case 'n':
            switch (s[1]) {
            case 'g': CMP(105); // nginx
            case 'i': CMP(108); // ninja
            }
            break;
        case 'p':
            switch (s[1]) {
            case 'a': CMP(111); // patch
            case 'r': CMP(123); // proto
            }
            break;
        case 's':
            switch (s[1]) {
            case 'c': CMP(137); // scala
            case 'l': CMP(144); // slice
            }
            break;
        case 't': CMP(156); // timer
        case 'v': CMP(165); // vcard
        case 'x': CMP(174); // xhtml
        }
        break;
    case 6:
        switch (s[0]) {
        case 'c': CMP(33); // coffee
        case 'd': CMP(48); // docker
        case 'e': CMP(53); // ebuild
        case 'g': CMP(72); // groovy
        case 's': CMP(146); // socket
        case 't': CMP(151); // target
        }
        break;
    case 7:
        switch (s[0]) {
        case 'd':
            switch (s[1]) {
            case 'e': CMP(43); // desktop
            case 'o': CMP(47); // docbook
            }
            break;
        case 'g':
            switch (s[1]) {
            case 'e':
                switch (s[2]) {
                case 'm': CMP(62); // gemspec
                case 'o': CMP(63); // geojson
                }
                break;
            case 'n': CMP(67); // gnuplot
            }
            break;
        case 's': CMP(141); // service
        case 't': CMP(155); // texinfo
        }
        break;
    case 8:
        switch (s[0]) {
        case 'm': CMP(96); // markdown
        case 'r': CMP(133); // rockspec
        case 't': CMP(158); // topojson
        }
        break;
    case 9:
        switch (s[0]) {
        case 'a': CMP(18); // automount
        case 'n': CMP(106); // nginxconf
        }
        break;
    case 10:
        switch (s[0]) {
        case 'f': CMP(58); // flatpakref
        case 'p': CMP(121); // postscript
        }
        break;
    case 11: CMP(59); // flatpakrepo
    }
    return 0;
compare:
    key = filetype_from_extension_table[idx].key;
    val = filetype_from_extension_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}

#undef CMP
