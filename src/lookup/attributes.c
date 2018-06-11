/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/lookup/attributes.gperf  */
/* Computed positions: -k'1,3' */
/* Filtered by: mk/gperf-filter.sed */

typedef struct {
    const char *const name;
    unsigned short attr;
} AttrHashSlot;

#define ATTRS_TOTAL_KEYWORDS 9
#define ATTRS_MIN_WORD_LENGTH 3
#define ATTRS_MAX_WORD_LENGTH 12
#define ATTRS_MIN_HASH_VALUE 0
#define ATTRS_MAX_HASH_VALUE 8
/* maximum key range = 9, duplicates = 0 */

inline
/*ARGSUSED*/
static unsigned int
attr_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 7, 1, 9,
      3, 4, 9, 9, 9, 1, 9, 3, 0, 3,
      9, 9, 9, 9, 2, 9, 9, 2, 2, 0,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
      9, 9, 9, 9, 9, 9
    };
  return asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[0]];
}

static const unsigned char attr_length_table[] =
  {
    12,  4,  5,  9,  7,  9,  3,  4,  6
  };

static const AttrHashSlot attr_table[] =
  {
    {"lowintensity", ATTR_DIM},
    {"bold", ATTR_BOLD},
    {"blink", ATTR_BLINK},
    {"invisible", ATTR_INVIS},
    {"reverse", ATTR_REVERSE},
    {"underline", ATTR_UNDERLINE},
    {"dim", ATTR_DIM},
    {"keep", ATTR_KEEP},
    {"italic", ATTR_ITALIC}
  };

static const AttrHashSlot*
lookup_attr (register const char *str, register size_t len)
{
  if (len <= ATTRS_MAX_WORD_LENGTH && len >= ATTRS_MIN_WORD_LENGTH)
    {
      register unsigned int key = attr_hash (str, len);

      if (key <= ATTRS_MAX_HASH_VALUE)
        if (len == attr_length_table[key])
          {
            register const char *s = attr_table[key].name;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &attr_table[key];
          }
    }
  return 0;
}
