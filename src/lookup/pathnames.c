/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/lookup/pathnames.gperf  */
/* Computed positions: -k'6' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 3, duplicates = 0 */

inline
/*ARGSUSED*/
static unsigned int
ft_pathname_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 2, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 1, 3, 0, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3
    };
  return asso_values[(unsigned char)str[5]];
}

static const FileTypeHashSlot*
filetype_from_pathname (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 3,
      MIN_WORD_LENGTH = 10,
      MAX_WORD_LENGTH = 19,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 2
    };

  static const unsigned char lengthtable[] =
    {
      10, 10, 19
    };
  static const FileTypeHashSlot ft_pathname_table[] =
    {
      {"/etc/hosts", CONFIG},
      {"/etc/fstab", CONFIG},
      {"/boot/grub/menu.lst", CONFIG}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_pathname_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = ft_pathname_table[key].key;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &ft_pathname_table[key];
          }
    }
  return 0;
}
