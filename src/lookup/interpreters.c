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

static FileTypeEnum filetype_from_interpreter(const char *s, size_t len)
{
    size_t idx;
    const char *key;
    FileTypeEnum val;
    switch (len) {
    case 2:
        idx = 41; // sh
        goto compare;
    case 3:
        switch (s[0]) {
        case 'a':
            switch (s[1]) {
            case 's':
                idx = 0; // ash
                goto compare;
            case 'w':
                idx = 1; // awk
                goto compare;
            }
            break;
        case 'c':
            idx = 4; // ccl
            goto compare;
        case 'e':
            idx = 10; // ecl
            goto compare;
        case 'k':
            idx = 18; // ksh
            goto compare;
        case 'l':
            idx = 20; // lua
            goto compare;
        case 'p':
            idx = 32; // php
            goto compare;
        case 's':
            idx = 40; // sed
            goto compare;
        case 't':
            idx = 42; // tcc
            goto compare;
        case 'z':
            idx = 45; // zsh
            goto compare;
        }
        break;
    case 4:
        switch (s[0]) {
        case 'b':
            idx = 2; // bash
            goto compare;
        case 'd':
            idx = 9; // dash
            goto compare;
        case 'g':
            switch (s[1]) {
            case 'a':
                idx = 11; // gawk
                goto compare;
            case 's':
                idx = 15; // gsed
                goto compare;
            }
            break;
        case 'l':
            idx = 19; // lisp
            goto compare;
        case 'm':
            switch (s[1]) {
            case 'a':
                switch (s[2]) {
                case 'k':
                    idx = 23; // make
                    goto compare;
                case 'w':
                    idx = 24; // mawk
                    goto compare;
                }
                break;
            case 'k':
                idx = 25; // mksh
                goto compare;
            case 'o':
                idx = 26; // moon
                goto compare;
            }
            break;
        case 'n':
            switch (s[1]) {
            case 'a':
                idx = 27; // nawk
                goto compare;
            case 'o':
                idx = 28; // node
                goto compare;
            }
            break;
        case 'p':
            idx = 31; // perl
            goto compare;
        case 'r':
            switch (s[1]) {
            case '6':
                idx = 34; // r6rs
                goto compare;
            case 'a':
                idx = 36; // rake
                goto compare;
            case 'u':
                idx = 37; // ruby
                goto compare;
            }
            break;
        case 's':
            idx = 39; // sbcl
            goto compare;
        case 'w':
            idx = 44; // wish
            goto compare;
        }
        break;
    case 5:
        switch (s[0]) {
        case 'c':
            idx = 6; // clisp
            goto compare;
        case 'g':
            switch (s[1]) {
            case 'm':
                idx = 12; // gmake
                goto compare;
            case 'u':
                idx = 16; // guile
                goto compare;
            }
            break;
        case 'j':
            idx = 17; // jruby
            goto compare;
        case 'p':
            idx = 30; // pdksh
            goto compare;
        case 't':
            idx = 43; // tclsh
            goto compare;
        }
        break;
    case 6:
        switch (s[0]) {
        case 'b':
            idx = 3; // bigloo
            goto compare;
        case 'c':
            idx = 7; // coffee
            goto compare;
        case 'g':
            idx = 14; // groovy
            goto compare;
        case 'l':
            idx = 21; // luajit
            goto compare;
        case 'p':
            idx = 33; // python
            goto compare;
        case 'r':
            idx = 35; // racket
            goto compare;
        }
        break;
    case 7:
        switch (s[0]) {
        case 'c':
            switch (s[1]) {
            case 'h':
                idx = 5; // chicken
                goto compare;
            case 'r':
                idx = 8; // crystal
                goto compare;
            }
            break;
        case 'g':
            idx = 13; // gnuplot
            goto compare;
        case 'm':
            idx = 22; // macruby
            goto compare;
        }
        break;
    case 10:
        switch (s[0]) {
        case 'o':
            idx = 29; // openrc-run
            goto compare;
        case 'r':
            idx = 38; // runhaskell
            goto compare;
        }
        break;
    }
    return 0;
compare:
    key = filetype_from_interpreter_table[idx].key;
    val = filetype_from_interpreter_table[idx].val;
    return memcmp(s, key, len) ? 0 : val;
}
