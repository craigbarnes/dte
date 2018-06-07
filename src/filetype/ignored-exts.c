/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/filetype/ignored-exts.gperf  */
/* Computed positions: -k'1,$' */
/* Filtered by: tools/gperf-filter.sed */
/* maximum key range = 11, duplicates = 0 */

inline
static unsigned int
ft_ignored_ext_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11,  5, 11,
       1,  3, 11,  0, 11, 11, 11,  5, 11, 11,
       8,  0,  3, 11,  4, 11,  7, 11, 11,  1,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
      11, 11, 11, 11, 11, 11
    };
  return asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

static const char*
is_ignored_extension (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 11,
      MIN_WORD_LENGTH = 3,
      MAX_WORD_LENGTH = 9,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 10
    };

  static const unsigned char lengthtable[] =
    {
       4,  3,  8,  7,  6,  6,  7,  7,  9,  3,  3
    };
  static const char * const wordlist[] =
    {
      "orig",
      "old",
      "dpkg-old",
      "pacorig",
      "pacnew",
      "rpmnew",
      "pacsave",
      "rpmsave",
      "dpkg-dist",
      "new",
      "bak"
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_ignored_ext_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key];

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return s;
          }
    }
  return 0;
}
