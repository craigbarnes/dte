/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/lookup/attributes.gperf  */
/* Computed positions: -k'1,3' */
/* Filtered by: mk/gperf-filter.sed */

typedef struct {
    const char *const name;
    unsigned short attr;
} AttrHashSlot;

#define ATTRS_TOTAL_KEYWORDS 8
#define ATTRS_MIN_WORD_LENGTH 4
#define ATTRS_MAX_WORD_LENGTH 12
#define ATTRS_MIN_HASH_VALUE 0
#define ATTRS_MAX_HASH_VALUE 7
/* maximum key range = 8, duplicates = 0 */

inline
/*ARGSUSED*/
static unsigned int
attr_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 6, 1, 8,
      3, 3, 8, 8, 8, 1, 8, 2, 0, 8,
      8, 8, 8, 8, 2, 8, 8, 3, 2, 0,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
      8, 8, 8, 8, 8, 8
    };
  return asso_values[(unsigned char)str[2]] + asso_values[(unsigned char)str[0]];
}

static const unsigned char attr_length_table[] =
  {
    12,  4,  5,  9,  7,  4,  9,  6
  };

static const AttrHashSlot attr_table[] =
  {
    {"lowintensity", ATTR_DIM},
    {"bold", ATTR_BOLD},
    {"blink", ATTR_BLINK},
    {"invisible", ATTR_INVIS},
    {"reverse", ATTR_REVERSE},
    {"keep", ATTR_KEEP},
    {"underline", ATTR_UNDERLINE},
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
