/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -D src/filetype/extensions.gperf  */
/* Computed positions: -k'1-2,$' */
/* Filtered by: tools/gperf-filter.sed */

#define EXTENSIONS_TOTAL_KEYWORDS 146
#define EXTENSIONS_MIN_WORD_LENGTH 1
#define EXTENSIONS_MAX_WORD_LENGTH 11
#define EXTENSIONS_MIN_HASH_VALUE 13
#define EXTENSIONS_MAX_HASH_VALUE 235
/* maximum key range = 223, duplicates = 0 */

inline
static unsigned int
ft_ext_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236,   7, 236,   6, 236, 236, 236,  47,
       40,   6,  35,  33,  28,  22,  18,  15, 236, 236,
      236, 236, 236, 236, 236, 236, 236,  10, 236, 236,
      236, 236,   9, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236,   7, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236,  40, 124,   8,
       31,   9,  42,  36,  34,  49,  96,  86,  20,  11,
       13,  62,  43,  24,  39,   6,   6,  98,  81,  20,
       76,  99,  19,   9, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236, 236, 236,
      236, 236, 236, 236, 236, 236, 236, 236
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[1]+2];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

static const FileTypeHashSlot*
filetype_from_extension (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       1,  1,  1,  1,  1,  1,  3,  4,  6,  2,  3,  3,  3,  1,
       4,  5,  3,  1,  3,  4,  8,  1,  6,  2,  2,  1,  5,  6,
       4,  8,  4,  3,  4,  2,  3,  1,  7,  3,  4,  3,  2,  1,
       4,  3,  1,  1,  1,  4,  4,  9,  3,  6,  2,  8,  1,  5,
       3,  5,  4,  2,  7,  4,  4,  5,  3,  7,  3,  3,  1,  5,
       6,  3,  4,  4,  3,  3, 10,  3,  4,  2,  4,  2,  9,  2,
       3,  5,  7,  2,  3,  3,  3,  5,  4,  2,  5,  3,  4, 11,
       3,  4,  3,  4,  4,  5,  5,  2,  4,  2,  3,  4,  5,  3,
       2,  6,  4,  3,  3,  2,  3,  4,  3,  4,  3,  2,  2,  4,
       4,  3,  3,  3,  3,  3,  3,  3,  3,  2,  5,  1,  2,  3,
       3,  4,  3,  3,  3,  2
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {"3", ROFF},
      {"S", ASSEMBLY},
      {"c", C},
      {"H", CPLUSPLUS},
      {"C", CPLUSPLUS},
      {"m", OBJECTIVEC},
      {"c++", CPLUSPLUS},
      {"scss", SCSS},
      {"target", INI},
      {"cc", CPLUSPLUS},
      {"sls", SCHEME},
      {"scm", SCHEME},
      {"cls", TEX},
      {"9", ROFF},
      {"make", MAKE},
      {"slice", INI},
      {"sql", SQL},
      {"8", ROFF},
      {"tcl", TCL},
      {"mkdn", MARKDOWN},
      {"markdown", MARKDOWN},
      {"l", LEX},
      {"socket", INI},
      {"cl", COMMONLISP},
      {"el", EMACSLISP},
      {"7", ROFF},
      {"mount", INI},
      {"coffee", COFFEESCRIPT},
      {"dart", DART},
      {"topojson", JSON},
      {"moon", MOONSCRIPT},
      {"sld", SCHEME},
      {"toml", TOML},
      {"cr", RUBY},
      {"mkd", MARKDOWN},
      {"6", ROFF},
      {"service", INI},
      {"rkt", RACKET},
      {"rake", RUBY},
      {"py3", PYTHON},
      {"pc", PKGCONFIG},
      {"d", D},
      {"page", MALLARD},
      {"pls", INI},
      {"5", ROFF},
      {"h", C},
      {"4", ROFF},
      {"glsl", GLSL},
      {"rktl", RACKET},
      {"automount", INI},
      {"sed", SED},
      {"ebuild", SHELL},
      {"pl", PERL},
      {"rockspec", LUA},
      {"2", ROFF},
      {"emacs", EMACSLISP},
      {"lua", LUA},
      {"cmake", CMAKE},
      {"rktd", RACKET},
      {"md", MARKDOWN},
      {"gemspec", RUBY},
      {"frag", GLSL},
      {"path", INI},
      {"patch", DIFF},
      {"ads", ADA},
      {"geojson", JSON},
      {"cpp", CPLUSPLUS},
      {"eml", EMAIL},
      {"1", ROFF},
      {"glslf", GLSL},
      {"docker", DOCKER},
      {"ins", TEX},
      {"doap", XML},
      {"perl", PERL},
      {"cmd", BATCHFILE},
      {"cxx", CPLUSPLUS},
      {"flatpakref", INI},
      {"mak", MAKE},
      {"mawk", AWK},
      {"mk", MAKE},
      {"nawk", AWK},
      {"ss", SCHEME},
      {"nginxconf", NGINX},
      {"cs", CSHARP},
      {"css", CSS},
      {"proto", PROTOBUF},
      {"desktop", INI},
      {"pm", PERL},
      {"hpp", CPLUSPLUS},
      {"clj", CLOJURE},
      {"tex", TEX},
      {"gperf", GPERF},
      {"cson", COFFEESCRIPT},
      {"go", GO},
      {"dterc", DTERC},
      {"rdf", XML},
      {"vert", GLSL},
      {"flatpakrepo", INI},
      {"htm", HTML},
      {"yaml", YAML},
      {"hxx", CPLUSPLUS},
      {"vala", VALA},
      {"gawk", AWK},
      {"glslv", GLSL},
      {"timer", INI},
      {"sh", SHELL},
      {"html", HTML},
      {"hs", HASKELL},
      {"bat", BATCHFILE},
      {"vapi", VALA},
      {"nginx", NGINX},
      {"ini", INI},
      {"rs", RUST},
      {"groovy", GROOVY},
      {"java", JAVA},
      {"auk", AWK},
      {"asm", ASSEMBLY},
      {"py", PYTHON},
      {"zsh", SHELL},
      {"doxy", DOXYGEN},
      {"xml", XML},
      {"diff", DIFF},
      {"lsp", COMMONLISP},
      {"hh", CPLUSPLUS},
      {"di", D},
      {"bash", SHELL},
      {"wsgi", PYTHON},
      {"asd", COMMONLISP},
      {"bbl", TEX},
      {"ltx", TEX},
      {"vim", VIML},
      {"yml", YAML},
      {"php", PHP},
      {"sty", TEX},
      {"dtx", TEX},
      {"rb", RUBY},
      {"xhtml", HTML},
      {"y", YACC},
      {"js", JAVASCRIPT},
      {"xsd", XML},
      {"adb", ADA},
      {"json", JSON},
      {"btm", BATCHFILE},
      {"ksh", SHELL},
      {"awk", AWK},
      {"ui", XML}
    };

  static const short lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,   0,  -1,   1,  -1,   2,  -1,   3,
       -1,   4,  -1,   5,   6,   7,   8,   9,  10,  11,
       12,  13,  14,  15,  -1,  16,  -1,  17,  18,  19,
       20,  21,  22,  23,  24,  25,  26,  27,  -1,  28,
       -1,  29,  30,  31,  32,  33,  34,  35,  36,  37,
       38,  39,  40,  41,  42,  43,  -1,  44,  -1,  45,
       -1,  46,  -1,  47,  48,  49,  50,  51,  52,  53,
       -1,  54,  55,  56,  57,  58,  59,  60,  61,  62,
       63,  64,  65,  66,  67,  68,  69,  -1,  -1,  -1,
       70,  71,  72,  73,  74,  -1,  75,  76,  77,  78,
       79,  80,  81,  82,  83,  84,  85,  86,  87,  88,
       89,  90,  91,  92,  93,  94,  95,  96,  97,  98,
       -1,  99, 100, 101, 102, 103, 104,  -1, 105, 106,
      107, 108, 109, 110, 111, 112,  -1, 113, 114, 115,
       -1,  -1, 116, 117, 118,  -1,  -1,  -1, 119,  -1,
       -1, 120,  -1, 121, 122,  -1, 123,  -1, 124,  -1,
      125, 126, 127,  -1,  -1,  -1,  -1,  -1, 128,  -1,
      129, 130,  -1,  -1, 131, 132,  -1,  -1,  -1, 133,
       -1, 134,  -1,  -1,  -1,  -1, 135, 136,  -1, 137,
       -1,  -1, 138,  -1,  -1,  -1,  -1,  -1, 139, 140,
       -1, 141,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 142,
       -1, 143,  -1,  -1,  -1,  -1,  -1,  -1, 144,  -1,
       -1,  -1,  -1,  -1,  -1, 145
    };

  if (len <= EXTENSIONS_MAX_WORD_LENGTH && len >= EXTENSIONS_MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_ext_hash (str, len);

      if (key <= EXTENSIONS_MAX_HASH_VALUE)
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
