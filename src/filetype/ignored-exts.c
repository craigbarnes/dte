/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -Dm50 src/filetype/ignored-exts.gperf  */
/* Computed positions: -k'1,4' */
/* Filtered by: tools/gperf-filter.sed */

#define IGNORED_EXTS_TOTAL_KEYWORDS 11
#define IGNORED_EXTS_MIN_WORD_LENGTH 3
#define IGNORED_EXTS_MAX_WORD_LENGTH 9
#define IGNORED_EXTS_MIN_HASH_VALUE 3
#define IGNORED_EXTS_MAX_HASH_VALUE 14
/* maximum key range = 12, duplicates = 0 */

inline
static unsigned int
ft_ignored_ext_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15,  1, 15,
       4, 15, 15,  0, 15, 15, 15, 15, 15, 15,
       0,  7,  0, 15,  1,  1, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
      15, 15, 15, 15, 15, 15
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const char*
is_ignored_extension (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       3,  3,  6,  6,  7,  7,  3,  4,  8,  9,  7
    };
  static const char * const wordlist[] =
    {
      "new",
      "bak",
      "pacnew",
      "rpmnew",
      "pacsave",
      "rpmsave",
      "old",
      "orig",
      "dpkg-old",
      "dpkg-dist",
      "pacorig"
    };

  static const signed char lookup[] =
    {
      -1, -1, -1,  0,  1, -1,  2,  3,  4,  5,  6,  7,  8,  9,
      10
    };

  if (len <= IGNORED_EXTS_MAX_WORD_LENGTH && len >= IGNORED_EXTS_MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_ignored_ext_hash (str, len);

      if (key <= IGNORED_EXTS_MAX_HASH_VALUE)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              if (len == lengthtable[index])
                {
                  register const char *s = wordlist[index];

                  if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
                    return s;
                }
            }
        }
    }
  return 0;
}
