static const struct {
    const char key[16];
    const FileTypeEnum val;
} filetype_from_interpreter_table[46] = {
    {"ash", SHELL},
    {"awk", AWK},
    {"bash", SHELL},
    {"bigloo", SCHEME},
    {"ccl", COMMONLISP},
    {"chicken", SCHEME},
    {"clisp", COMMONLISP},
    {"coffee", COFFEESCRIPT},
    {"crystal", RUBY},
    {"dash", SHELL},
    {"ecl", COMMONLISP},
    {"gawk", AWK},
    {"gmake", MAKE},
    {"gnuplot", GNUPLOT},
    {"groovy", GROOVY},
    {"gsed", SED},
    {"guile", SCHEME},
    {"jruby", RUBY},
    {"ksh", SHELL},
    {"lisp", COMMONLISP},
    {"lua", LUA},
    {"luajit", LUA},
    {"macruby", RUBY},
    {"make", MAKE},
    {"mawk", AWK},
    {"mksh", SHELL},
    {"moon", MOONSCRIPT},
    {"nawk", AWK},
    {"node", JAVASCRIPT},
    {"openrc-run", SHELL},
    {"pdksh", SHELL},
    {"perl", PERL},
    {"php", PHP},
    {"python", PYTHON},
    {"r6rs", SCHEME},
    {"racket", SCHEME},
    {"rake", RUBY},
    {"ruby", RUBY},
    {"runhaskell", HASKELL},
    {"sbcl", COMMONLISP},
    {"sed", SED},
    {"sh", SHELL},
    {"tcc", C},
    {"tclsh", TCL},
    {"wish", TCL},
    {"zsh", SHELL},
};

#define CMP(i) idx = i; goto compare

static FileTypeEnum filetype_from_interpreter(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 2: CMP(41); // sh
    case 3:
        switch (s[0]) {
        case 'a':
            switch (s[1]) {
            case 's': CMP(0); // ash
            case 'w': CMP(1); // awk
            }
            break;
        case 'c': CMP(4); // ccl
        case 'e': CMP(10); // ecl
        case 'k': CMP(18); // ksh
        case 'l': CMP(20); // lua
        case 'p': CMP(32); // php
        case 's': CMP(40); // sed
        case 't': CMP(42); // tcc
        case 'z': CMP(45); // zsh
        }
        break;
    case 4:
        switch (s[0]) {
        case 'b': CMP(2); // bash
        case 'd': CMP(9); // dash
        case 'g':
            switch (s[1]) {
            case 'a': CMP(11); // gawk
            case 's': CMP(15); // gsed
            }
            break;
        case 'l': CMP(19); // lisp
        case 'm':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'k': CMP(23); // make
                case 'w': CMP(24); // mawk
                }
                break;
            case 'k': CMP(25); // mksh
            case 'o': CMP(26); // moon
            }
            break;
        case 'n':
            switch (s[1]) {
            case 'a': CMP(27); // nawk
            case 'o': CMP(28); // node
            }
            break;
        case 'p': CMP(31); // perl
        case 'r':
            switch (s[1]) {
            case '6': CMP(34); // r6rs
            case 'a': CMP(36); // rake
            case 'u': CMP(37); // ruby
            }
            break;
        case 's': CMP(39); // sbcl
        case 'w': CMP(44); // wish
        }
        break;
    case 5:
        switch (s[0]) {
        case 'c': CMP(6); // clisp
        case 'g':
            switch (s[1]) {
            case 'm': CMP(12); // gmake
            case 'u': CMP(16); // guile
            }
            break;
        case 'j': CMP(17); // jruby
        case 'p': CMP(30); // pdksh
        case 't': CMP(43); // tclsh
        }
        break;
    case 6:
        switch (s[0]) {
        case 'b': CMP(3); // bigloo
        case 'c': CMP(7); // coffee
        case 'g': CMP(14); // groovy
        case 'l': CMP(21); // luajit
        case 'p': CMP(33); // python
        case 'r': CMP(35); // racket
        }
        break;
    case 7:
        switch (s[0]) {
        case 'c':
            switch (s[1]) {
            case 'h': CMP(5); // chicken
            case 'r': CMP(8); // crystal
            }
            break;
        case 'g': CMP(13); // gnuplot
        case 'm': CMP(22); // macruby
        }
        break;
    case 10:
        switch (s[0]) {
        case 'o': CMP(29); // openrc-run
        case 'r': CMP(38); // runhaskell
        }
        break;
    }
    return 0;
compare:
    key = filetype_from_interpreter_table[idx].key;
    val = filetype_from_interpreter_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}

#undef CMP
