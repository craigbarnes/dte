/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/lookup/colors.gperf  */
/* Computed positions: -k'1,4,6' */
/* Filtered by: tools/gperf-filter.sed */

typedef struct {
    const char *const name;
    short color;
} ColorHashSlot;

#define COLORS_TOTAL_KEYWORDS 18
#define COLORS_MIN_WORD_LENGTH 3
#define COLORS_MAX_WORD_LENGTH 12
#define COLORS_MIN_HASH_VALUE 2
#define COLORS_MAX_HASH_VALUE 19
/* maximum key range = 18, duplicates = 0 */

inline
static unsigned int
color_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 10,  5, 11,
       8,  4, 20,  4,  1, 20, 20,  8,  1, 10,
       3, 20,  7, 20,  2, 20,  3, 20, 20,  8,
      20,  1, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
      20, 20, 20, 20, 20, 20
    };
  register unsigned int hval = 0;

  switch (len)
    {
      default:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
      case 4:
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

static const unsigned char color_length_table[] =
  {
     0,  0,  3, 11,  8,  4, 10,  9,  5,  4,  6,  5, 12,  9,
     4,  4,  5,  7,  8,  7
  };

static const ColorHashSlot color_table[] =
  {
    {(char*)0,-2}, {(char*)0,-2},
    {"red", 1},
    {"lightyellow", 11},
    {"lightred", 9},
    {"gray", 7},
    {"lightgreen", 10},
    {"lightblue", 12},
    {"green", 2},
    {"blue", 4},
    {"yellow", 3},
    {"white", 15},
    {"lightmagenta", 13},
    {"lightcyan", 14},
    {"cyan", 6},
    {"keep", -2},
    {"black", 0},
    {"magenta", 5},
    {"darkgray", 8},
    {"default", -1}
  };

static const ColorHashSlot*
lookup_color (register const char *str, register size_t len)
{
  if (len <= COLORS_MAX_WORD_LENGTH && len >= COLORS_MIN_WORD_LENGTH)
    {
      register unsigned int key = color_hash (str, len);

      if (key <= COLORS_MAX_HASH_VALUE)
        if (len == color_length_table[key])
          {
            register const char *s = color_table[key].name;

            if (s && *str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &color_table[key];
          }
    }
  return 0;
}
