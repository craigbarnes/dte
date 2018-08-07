/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m75 -n src/lookup/config-cmds.gperf  */
/* Computed positions: -k'1,4' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 11, duplicates = 0 */

inline
static unsigned int
hash_config_command (register const char *str, register size_t len)
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
      11, 11, 11, 11, 11, 11, 11,  5,  4,  8,
       5,  1,  6, 11,  5,  2, 11, 11,  2, 11,
      11,  1, 11, 11, 11,  0, 11, 11, 11, 11,
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
  register unsigned int hval = 0;

  switch (len)
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
lookup_config_command (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 11,
      MIN_WORD_LENGTH = 2,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 10
    };

  static const unsigned char lengthtable[] =
    {
       3,  6,  8,  6,  7,  2,  2, 11,  2,  4,  5
    };
  static const char * const wordlist[] =
    {
      "set",
      "setenv",
      "errorfmt",
      "option",
      "include",
      "hi",
      "ft",
      "load-syntax",
      "cd",
      "bind",
      "alias"
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash_config_command (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key];

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return s;
          }
    }
  return 0;
}
