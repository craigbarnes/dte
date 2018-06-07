/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -Dm50 src/filetype/interpreters.gperf  */
/* Computed positions: -k'1,3,7' */
/* Filtered by: tools/gperf-filter.sed */

#define INTERPRETERS_TOTAL_KEYWORDS 43
#define INTERPRETERS_MIN_WORD_LENGTH 2
#define INTERPRETERS_MAX_WORD_LENGTH 10
#define INTERPRETERS_MIN_HASH_VALUE 2
#define INTERPRETERS_MAX_HASH_VALUE 51
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

static const FileTypeHashSlot *
filetype_from_interpreter (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       2,  4,  3,  4,  6,  3,  4,  5,  4,  5,  3,  3,  3,  6,
       4,  4,  6,  5,  4,  5,  4,  5,  6,  3,  4,  4,  7,  4,
       4,  4,  4,  5,  7,  3,  3,  3,  6,  3,  7,  7,  7,  6,
      10
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {"sh", SHELL},
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
      {"openrc-run", SHELL}
    };

  static const signed char lookup[] =
    {
      -1, -1,  0, -1,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
      11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
      25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
      39, 40, 41, -1, -1, -1, -1, -1, -1, 42
    };

  if (len <= INTERPRETERS_MAX_WORD_LENGTH && len >= INTERPRETERS_MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_interpreter_hash (str, len);

      if (key <= INTERPRETERS_MAX_HASH_VALUE)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              if (len == lengthtable[index])
                {
                  register const char *s = wordlist[index].key;

                  if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
                    return &wordlist[index];
                }
            }
        }
    }
  return 0;
}
