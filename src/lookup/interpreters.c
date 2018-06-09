/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/lookup/interpreters.gperf  */
/* Computed positions: -k'1,3' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 53, duplicates = 0 */

inline
static unsigned int
ft_interpreter_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55,  7, 23,  0,
      30, 25, 35,  6, 27, 21,  0,  6, 11,  3,
       4, 19,  0, 55,  2,  0, 14,  0, 55, 15,
      55,  2, 10, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
      55, 55, 55, 55, 55, 55
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
      TOTAL_KEYWORDS = 41,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 10,
      MIN_HASH_VALUE = 2,
      MAX_HASH_VALUE = 54
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  2,  3,  4,  5,  4,  4,  6,  7,  7,  5,  4,  4,
       3,  4,  3,  3,  5,  4,  6,  3,  4,  4,  6,  4,  5,  4,
       7,  4,  5,  6,  5,  3,  4,  6,  3,  3,  4,  3,  3,  6,
       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 10
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {(char*)0,0}, {(char*)0,0},
      {"sh", SHELL},
      {"php", PHP},
      {"sbcl", COMMONLISP},
      {"jruby", RUBY},
      {"perl", PERL},
      {"mksh", SHELL},
      {"racket", SCHEME},
      {"crystal", RUBY},
      {"macruby", RUBY},
      {"pdksh", SHELL},
      {"rake", RUBY},
      {"make", MAKE},
      {"ccl", COMMONLISP},
      {"lisp", COMMONLISP},
      {"awk", AWK},
      {"tcc", C},
      {"gmake", MAKE},
      {"wish", TCL},
      {"python", PYTHON},
      {"lua", LUA},
      {"mawk", AWK},
      {"nawk", AWK},
      {"luajit", LUA},
      {"gawk", AWK},
      {"clisp", COMMONLISP},
      {"bash", SHELL},
      {"chicken", SCHEME},
      {"ruby", RUBY},
      {"tclsh", TCL},
      {"groovy", GROOVY},
      {"guile", SCHEME},
      {"sed", SED},
      {"dash", SHELL},
      {"bigloo", SCHEME},
      {"ksh", SHELL},
      {"ash", SHELL},
      {"node", JAVASCRIPT},
      {"ecl", COMMONLISP},
      {"zsh", SHELL},
      {"coffee", COFFEESCRIPT},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
      {(char*)0,0}, {(char*)0,0}, {(char*)0,0}, {(char*)0,0},
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
