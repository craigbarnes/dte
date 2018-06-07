/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/filetype/interpreters.gperf  */
/* Computed positions: -k'1,3,7' */
/* Filtered by: tools/gperf-filter.sed */
/* maximum key range = 50, duplicates = 0 */

inline
static unsigned int
ft_interpreter_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52,  0, 52, 52, 52, 52,
      22, 21, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52,  5, 28,  0,
      23, 32, 33, 10, 28, 19,  3,  6,  5, 13,
       3,  9,  1, 52,  1,  0, 12,  2, 52, 14,
      52, 23,  7, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
      52, 52, 52, 52, 52, 52
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
      case 5:
      case 4:
      case 3:
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
      TOTAL_KEYWORDS = 43,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 10,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 51
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  2,  0,  4,  3,  4,  6,  3,  4,  5,  4,  5,  3,
       3,  3,  6,  4,  4,  6,  5,  4,  5,  4,  5,  6,  3,  4,
       4,  7,  4,  4,  4,  4,  5,  7,  3,  3,  3,  6,  3,  7,
       7,  7,  6,  0,  0,  0,  0,  0,  0, 10
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {(char*)0,0}, {(char*)0,0},
      {"sh", SHELL},
      {(char*)0,0},
      {"sbcl", COMMONLISP},
      {"php", PHP},
      {"perl", PERL},
      {"racket", SCHEME},
      {"ccl", COMMONLISP},
      {"lisp", COMMONLISP},
      {"jruby", RUBY},
      {"rake", RUBY},
      {"pdksh", SHELL},
      {"lua", LUA},
      {"awk", AWK},
      {"tcc", C},
      {"luajit", LUA},
      {"mksh", SHELL},
      {"wish", TCL},
      {"python", PYTHON},
      {"gmake", MAKE},
      {"nawk", AWK},
      {"tclsh", TCL},
      {"make", MAKE},
      {"clisp", COMMONLISP},
      {"groovy", GROOVY},
      {"sed", SED},
      {"dash", SHELL},
      {"gawk", AWK},
      {"chicken", SCHEME},
      {"node", JAVASCRIPT},
      {"mawk", AWK},
      {"bash", SHELL},
      {"ruby", RUBY},
      {"guile", SCHEME},
      {"crystal", RUBY},
      {"ash", SHELL},
      {"ksh", SHELL},
      {"zsh", SHELL},
      {"coffee", COFFEESCRIPT},
      {"ecl", COMMONLISP},
      {"python3", PYTHON},
      {"python2", PYTHON},
      {"macruby", RUBY},
      {"bigloo", SCHEME},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {(char*)0,0}, {(char*)0,0},
      {"openrc-run", SHELL}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_interpreter_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].key;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
