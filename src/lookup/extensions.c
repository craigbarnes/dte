/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -D src/lookup/extensions.gperf  */
/* Computed positions: -k'1-2,$' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 270, duplicates = 0 */

inline
static unsigned int
filetype_extension_hash (register const char *str, register size_t len)
{
  static const unsigned short asso_values[] =
    {
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273,   3, 273,   2, 273, 273, 273,  36,
       29,   2,  28,  27,  25,  22,  17,  16, 273, 273,
      273, 273, 273, 273, 273, 273, 273,  12, 273, 273,
      273, 273,  11, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273,   5, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273,  71, 112,   4,
       14,   8,  77,  30,  63,  97, 130,  91,  26,   1,
        2,  66,  33,  14,  48,   1,   6,  82,  84,  30,
       27, 103,  27,   3, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273, 273, 273,
      273, 273, 273, 273, 273, 273, 273, 273
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
  enum
    {
      TOTAL_KEYWORDS = 164,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 3,
      MAX_HASH_VALUE = 272
    };

  static const unsigned char lengthtable[] =
    {
       1,  1,  3,  4,  1,  3,  1,  3,  3,  4,  8,  5,  4,  2,
       3,  3,  4,  6,  1,  1,  5,  6,  4,  1,  8,  3,  6,  1,
       2,  1,  3,  2,  3,  3,  6,  3,  3,  1,  7,  2,  3,  4,
       4,  1,  1,  1,  3,  1,  3,  1,  2,  3,  4,  2,  4,  4,
       3,  4,  7,  7,  1,  8,  7,  4,  5,  6,  5,  7,  5,  2,
       3,  3,  2,  3,  2,  4,  4,  2,  2,  3,  3,  4,  4,  2,
       3,  4,  5,  5,  5,  2,  2,  5,  2,  9,  4,  3,  4,  3,
       5,  3,  3,  4,  3,  3,  1,  3,  4,  3,  5,  2,  3,  4,
       4,  3,  3,  3,  2,  2,  3,  6,  3,  2,  5,  3,  3,  3,
      11,  3,  5,  4, 10,  5,  3,  3,  3,  2,  4,  3,  3,  4,
       9,  4,  5,  4,  3,  3,  2,  3,  3,  3,  2,  3,  1,  4,
       4,  2,  4,  3,  3,  3,  2,  3,  3,  2
    };
  static const FileTypeHashSlot filetype_extension_table[] =
    {
      {"m", OBJECTIVEC},
      {"3", ROFF},
      {"sls", SCHEME},
      {"mkdn", MARKDOWN},
      {"c", C},
      {"cls", TEX},
      {"S", ASSEMBLY},
      {"c++", CPLUSPLUS},
      {"scm", SCHEME},
      {"scss", SCSS},
      {"markdown", MARKDOWN},
      {"slice", INI},
      {"make", MAKE},
      {"cc", CPLUSPLUS},
      {"mkd", MARKDOWN},
      {"sld", SCHEME},
      {"moon", MOONSCRIPT},
      {"target", INI},
      {"H", CPLUSPLUS},
      {"C", CPLUSPLUS},
      {"mount", INI},
      {"socket", INI},
      {"dart", DART},
      {"d", D},
      {"topojson", JSON},
      {"sql", SQL},
      {"coffee", COFFEESCRIPT},
      {"9", ROFF},
      {"cl", COMMONLISP},
      {"8", ROFF},
      {"dot", DOT},
      {"el", EMACSLISP},
      {"pls", INI},
      {"py3", PYTHON},
      {"ebuild", SHELL},
      {"tcl", TCL},
      {"plt", GNUPLOT},
      {"7", ROFF},
      {"service", INI},
      {"pc", PKGCONFIG},
      {"sed", SED},
      {"page", MALLARD},
      {"toml", TOML},
      {"6", ROFF},
      {"l", LEX},
      {"5", ROFF},
      {"pot", GETTEXT},
      {"4", ROFF},
      {"rkt", RACKET},
      {"2", ROFF},
      {"cr", RUBY},
      {"cxx", CPLUSPLUS},
      {"glsl", GLSL},
      {"pl", PERL},
      {"rake", RUBY},
      {"doap", XML},
      {"tex", TEX},
      {"rktd", RACKET},
      {"geojson", JSON},
      {"gemspec", RUBY},
      {"1", ROFF},
      {"rockspec", LUA},
      {"gnuplot", GNUPLOT},
      {"rktl", RACKET},
      {"emacs", EMACSLISP},
      {"docker", DOCKER},
      {"cmake", CMAKE},
      {"desktop", INI},
      {"scala", SCALA},
      {"ss", SCHEME},
      {"cmd", BATCHFILE},
      {"cpp", CPLUSPLUS},
      {"cs", CSHARP},
      {"css", CSS},
      {"ts", TYPESCRIPT},
      {"cson", COFFEESCRIPT},
      {"perl", PERL},
      {"md", MARKDOWN},
      {"mk", MAKE},
      {"nim", NIM},
      {"mak", MAKE},
      {"mawk", AWK},
      {"nawk", AWK},
      {"pm", PERL},
      {"eml", EMAIL},
      {"path", INI},
      {"patch", DIFF},
      {"dterc", DTERC},
      {"proto", PROTOBUF},
      {"go", GO},
      {"gp", GNUPLOT},
      {"glslf", GLSL},
      {"po", GETTEXT},
      {"automount", INI},
      {"frag", GLSL},
      {"tsx", TYPESCRIPT},
      {"xslt", XML},
      {"hxx", CPLUSPLUS},
      {"glslv", GLSL},
      {"xml", XML},
      {"nix", NIX},
      {"vert", GLSL},
      {"bat", BATCHFILE},
      {"xsd", XML},
      {"h", C},
      {"dtx", TEX},
      {"gawk", AWK},
      {"lua", LUA},
      {"nginx", NGINX},
      {"rs", RUST},
      {"ins", TEX},
      {"doxy", DOXYGEN},
      {"yaml", YAML},
      {"xsl", XML},
      {"clj", CLOJURE},
      {"ltx", TEX},
      {"py", PYTHON},
      {"gv", DOT},
      {"lsp", COMMONLISP},
      {"groovy", GROOVY},
      {"hpp", CPLUSPLUS},
      {"hs", HASKELL},
      {"timer", INI},
      {"htm", HTML},
      {"ads", ADA},
      {"bbl", TEX},
      {"flatpakrepo", INI},
      {"asm", ASSEMBLY},
      {"gperf", GPERF},
      {"vala", VALA},
      {"flatpakref", INI},
      {"ninja", NINJA},
      {"asd", COMMONLISP},
      {"csv", CSV},
      {"zsh", SHELL},
      {"rb", RUBY},
      {"html", HTML},
      {"gpi", GNUPLOT},
      {"vim", VIML},
      {"bash", SHELL},
      {"nginxconf", NGINX},
      {"diff", DIFF},
      {"xhtml", HTML},
      {"vapi", VALA},
      {"sty", TEX},
      {"auk", AWK},
      {"sh", SHELL},
      {"yml", YAML},
      {"php", PHP},
      {"btm", BATCHFILE},
      {"di", D},
      {"rdf", XML},
      {"y", YACC},
      {"java", JAVA},
      {"wsgi", PYTHON},
      {"js", JAVASCRIPT},
      {"json", JSON},
      {"ada", ADA},
      {"ini", INI},
      {"ksh", SHELL},
      {"hh", CPLUSPLUS},
      {"adb", ADA},
      {"awk", AWK},
      {"ui", XML}
    };

  static const short lookup[] =
    {
       -1,  -1,  -1,   0,  -1,   1,  -1,   2,   3,   4,
        5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
       15,  16,  17,  18,  -1,  19,  20,  21,  22,  23,
       24,  25,  26,  27,  28,  29,  -1,  30,  31,  32,
       -1,  33,  34,  35,  36,  37,  38,  39,  40,  41,
       42,  43,  -1,  44,  -1,  45,  46,  47,  48,  49,
       50,  51,  52,  53,  54,  55,  56,  57,  -1,  58,
       -1,  59,  -1,  60,  61,  -1,  62,  -1,  -1,  63,
       64,  -1,  65,  66,  67,  68,  69,  70,  71,  72,
       73,  74,  75,  76,  77,  78,  -1,  79,  -1,  80,
       81,  82,  83,  84,  85,  86,  -1,  87,  -1,  -1,
       88,  -1,  89,  90,  91,  92,  93,  94,  95,  96,
       97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
      107, 108,  -1, 109, 110, 111,  -1, 112, 113, 114,
      115, 116,  -1, 117, 118, 119,  -1, 120, 121,  -1,
      122, 123, 124,  -1,  -1, 125, 126, 127,  -1,  -1,
      128,  -1,  -1, 129,  -1,  -1, 130,  -1,  -1, 131,
      132,  -1,  -1, 133,  -1, 134, 135, 136, 137, 138,
       -1,  -1,  -1, 139,  -1, 140, 141,  -1, 142, 143,
       -1, 144,  -1,  -1,  -1, 145, 146,  -1, 147, 148,
      149,  -1,  -1,  -1, 150, 151,  -1, 152,  -1, 153,
       -1,  -1,  -1, 154,  -1, 155,  -1,  -1, 156,  -1,
       -1,  -1, 157,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      158,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 159,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 160,  -1,
       -1,  -1,  -1, 161,  -1,  -1,  -1,  -1, 162,  -1,
       -1,  -1, 163
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = filetype_extension_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              if (len == lengthtable[index])
                {
                  register const char *s = filetype_extension_table[index].key;

                  if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
                    return &filetype_extension_table[index];
                }
            }
        }
    }
  return 0;
}
