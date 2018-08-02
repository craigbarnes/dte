/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/lookup/interpreters.gperf  */
/* Computed positions: -k'1,3' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 59, duplicates = 0 */

inline
static unsigned int
filetype_interpreter_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62,  9, 35,  3,
      33, 20,  4,  9, 31, 18, 12,  7, 12,  5,
      24, 21,  2, 62,  0,  1, 14, 29, 62, 16,
      62,  2,  8, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62, 62, 62, 62, 62,
      62, 62, 62, 62, 62, 62
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const FileTypeHashSlot*
filetype_from_interpreter (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 46,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 10,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 61
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  2,  4,  0,  4,  3,  4,  6,  4,  4,  7,  6,
       5,  7,  4,  4,  3,  3,  3,  4,  6,  5,  3,  4,  5,  6,
       7,  4,  4,  5,  5,  4, 10,  3,  6,  3,  4,  4,  4,  3,
       3,  3,  4,  7,  5,  0,  0,  0,  6, 10,  0,  0,  0,  0,
       0,  0,  0,  0,  0,  4
    };
  static const FileTypeHashSlot filetype_interpreter_table[] =
    {
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {"sh", SHELL},
      {"r6rs", SCHEME},
      {(char*)0,0},
      {"perl", PERL},
      {"php", PHP},
      {"sbcl", COMMONLISP},
      {"racket", SCHEME},
      {"mksh", SHELL},
      {"rake", RUBY},
      {"crystal", RUBY},
      {"coffee", COFFEESCRIPT},
      {"pdksh", SHELL},
      {"macruby", RUBY},
      {"make", MAKE},
      {"lisp", COMMONLISP},
      {"ccl", COMMONLISP},
      {"awk", AWK},
      {"tcc", C},
      {"wish", TCL},
      {"python", PYTHON},
      {"gmake", MAKE},
      {"lua", LUA},
      {"mawk", AWK},
      {"clisp", COMMONLISP},
      {"luajit", LUA},
      {"chicken", SCHEME},
      {"gawk", AWK},
      {"moon", MOONSCRIPT},
      {"tclsh", TCL},
      {"guile", SCHEME},
      {"gsed", SED},
      {"runhaskell", HASKELL},
      {"ecl", COMMONLISP},
      {"groovy", GROOVY},
      {"sed", SED},
      {"dash", SHELL},
      {"ruby", RUBY},
      {"bash", SHELL},
      {"ksh", SHELL},
      {"zsh", SHELL},
      {"ash", SHELL},
      {"nawk", AWK},
      {"gnuplot", GNUPLOT},
      {"jruby", RUBY},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {"bigloo", SCHEME},
      {"openrc-run", SHELL},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {(char*)0,0},
      {"node", JAVASCRIPT}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = filetype_interpreter_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = filetype_interpreter_table[key].key;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &filetype_interpreter_table[key];
          }
    }
  return 0;
}
