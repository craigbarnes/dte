/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m75 -D src/lookup/extensions.gperf  */
/* Computed positions: -k'1-2,$' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 338, duplicates = 0 */

inline
static unsigned int
filetype_extension_hash (register const char *str, register size_t len)
{
  static const unsigned short asso_values[] =
    {
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369,  16, 369,  16, 369, 369, 369,  34,
       33,  18,  19,  32,  16,  26,  25,  24, 369, 369,
      369, 369, 369, 369, 369, 369, 369,  23, 369, 369,
      369, 369,  22, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369,  21, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 123,  61,  17,
       31,  20, 119,  55, 105,  90, 100, 103,  45,  20,
       15,  96,  44,  23,  52,  15,  24,  88,  75,  43,
       45, 130,  22,  36, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369, 369, 369,
      369, 369, 369, 369, 369, 369, 369, 369
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
      TOTAL_KEYWORDS = 177,
      MIN_WORD_LENGTH = 1,
      MAX_WORD_LENGTH = 11,
      MIN_HASH_VALUE = 31,
      MAX_HASH_VALUE = 368
    };

  static const unsigned char lengthtable[] =
    {
       1,  1,  1,  1,  1,  1,  1,  1,  1,  3,  1,  3,  1,  3,
       1,  4,  5,  2,  2,  3,  4,  8,  4,  4,  1,  3,  1,  6,
       1,  6,  1,  8,  6,  5,  3,  4,  3,  3,  2,  3,  2,  2,
       4,  3,  3,  6,  1,  3,  4,  3,  2,  4,  7,  3,  8,  3,
       4,  3,  3,  2,  4,  6,  3,  3,  4,  2,  4,  2,  3,  4,
       3,  5,  2,  7,  5,  7,  7,  3,  5,  7,  5,  4,  3,  3,
       3,  4,  2,  2,  3,  4,  5,  1,  3,  2,  3,  5,  2,  4,
       3,  3,  4,  2,  5,  3,  2,  3,  3,  3,  5,  4,  5,  2,
       4,  3,  2,  2,  4,  3,  3,  7,  3,  5,  3,  4,  4,  4,
       3,  3,  5,  5,  4,  9,  3,  3,  4,  3,  3,  2,  4,  3,
       2,  1,  2,  3,  6,  3,  3,  4,  2,  3,  4,  4,  2,  3,
       3,  4,  5,  9,  3, 11,  4,  3,  5,  4,  3,  1, 10,  3,
       3,  2,  2,  3,  3,  3,  2,  3,  3
    };
  static const FileTypeHashSlot filetype_extension_table[] =
    {
      {"s", ASSEMBLY},
      {"6", ROFF},
      {"c", C},
      {"3", ROFF},
      {"4", ROFF},
      {"m", OBJECTIVEC},
      {"S", ASSEMBLY},
      {"H", CPLUSPLUS},
      {"C", CPLUSPLUS},
      {"sls", SCHEME},
      {"9", ROFF},
      {"cls", TEX},
      {"8", ROFF},
      {"c++", CPLUSPLUS},
      {"7", ROFF},
      {"scss", SCSS},
      {"slice", INI},
      {"cc", CPLUSPLUS},
      {"m4", M4},
      {"scm", SCHEME},
      {"mkdn", MARKDOWN},
      {"markdown", MARKDOWN},
      {"make", MAKE},
      {"moon", MOONSCRIPT},
      {"d", D},
      {"sld", SCHEME},
      {"5", ROFF},
      {"coffee", COFFEESCRIPT},
      {"2", ROFF},
      {"socket", INI},
      {"1", ROFF},
      {"topojson", JSON},
      {"target", INI},
      {"mount", INI},
      {"mkd", MARKDOWN},
      {"dart", DART},
      {"pls", INI},
      {"sql", SQL},
      {"cl", COMMONLISP},
      {"dot", DOT},
      {"el", EMACSLISP},
      {"pc", PKGCONFIG},
      {"page", MALLARD},
      {"plt", GNUPLOT},
      {"cxx", CPLUSPLUS},
      {"ebuild", SHELL},
      {"l", LEX},
      {"tcl", TCL},
      {"rake", RUBY},
      {"pot", GETTEXT},
      {"cr", RUBY},
      {"toml", TOML},
      {"service", INI},
      {"rkt", RACKET},
      {"rockspec", LUA},
      {"py3", PYTHON},
      {"doap", XML},
      {"sed", SED},
      {"bat", BATCHFILE},
      {"pl", PERL},
      {"rktd", RACKET},
      {"docker", DOCKER},
      {"cpp", CPLUSPLUS},
      {"svg", XML},
      {"glsl", GLSL},
      {"ss", SCHEME},
      {"rktl", RACKET},
      {"cs", CSHARP},
      {"css", CSS},
      {"cson", COFFEESCRIPT},
      {"tex", TEX},
      {"dterc", DTERC},
      {"ts", TYPESCRIPT},
      {"gnuplot", GNUPLOT},
      {"vcard", VCARD},
      {"geojson", JSON},
      {"gemspec", RUBY},
      {"clj", CLOJURE},
      {"emacs", EMACSLISP},
      {"desktop", INI},
      {"cmake", CMAKE},
      {"nawk", AWK},
      {"bbl", TEX},
      {"nim", NIM},
      {"mak", MAKE},
      {"mawk", AWK},
      {"mk", MAKE},
      {"rb", RUBY},
      {"cmd", BATCHFILE},
      {"perl", PERL},
      {"glslv", GLSL},
      {"v", VERILOG},
      {"ins", TEX},
      {"gp", GNUPLOT},
      {"dtx", TEX},
      {"nginx", NGINX},
      {"rs", RUST},
      {"vert", GLSL},
      {"btm", BATCHFILE},
      {"tsx", TYPESCRIPT},
      {"xslt", XML},
      {"pm", PERL},
      {"scala", SCALA},
      {"eml", EMAIL},
      {"po", GETTEXT},
      {"nix", NIX},
      {"xsd", XML},
      {"ltx", TEX},
      {"proto", PROTOBUF},
      {"path", INI},
      {"patch", DIFF},
      {"md", MARKDOWN},
      {"texi", TEXINFO},
      {"hxx", CPLUSPLUS},
      {"go", GO},
      {"gv", DOT},
      {"gawk", AWK},
      {"lsp", COMMONLISP},
      {"xsl", XML},
      {"texinfo", TEXINFO},
      {"csv", CSV},
      {"timer", INI},
      {"ver", VERILOG},
      {"vapi", VALA},
      {"bash", SHELL},
      {"doxy", DOXYGEN},
      {"xml", XML},
      {"php", PHP},
      {"glslf", GLSL},
      {"xhtml", HTML},
      {"yaml", YAML},
      {"automount", INI},
      {"gpi", GNUPLOT},
      {"vim", VIML},
      {"frag", GLSL},
      {"htm", HTML},
      {"hpp", CPLUSPLUS},
      {"js", JAVASCRIPT},
      {"json", JSON},
      {"vhd", VHDL},
      {"hs", HASKELL},
      {"h", C},
      {"py", PYTHON},
      {"lua", LUA},
      {"groovy", GROOVY},
      {"vcf", VCARD},
      {"zsh", SHELL},
      {"vala", VALA},
      {"sh", SHELL},
      {"sty", TEX},
      {"vhdl", VHDL},
      {"wsgi", PYTHON},
      {"di", D},
      {"ini", INI},
      {"bib", BIBTEX},
      {"html", HTML},
      {"gperf", GPERF},
      {"nginxconf", NGINX},
      {"asm", ASSEMBLY},
      {"flatpakrepo", INI},
      {"java", JAVA},
      {"asd", COMMONLISP},
      {"ninja", NINJA},
      {"diff", DIFF},
      {"ads", ADA},
      {"y", YACC},
      {"flatpakref", INI},
      {"auk", AWK},
      {"yml", YAML},
      {"vh", VHDL},
      {"ui", XML},
      {"rdf", XML},
      {"ksh", SHELL},
      {"adb", ADA},
      {"hh", CPLUSPLUS},
      {"awk", AWK},
      {"ada", ADA}
    };

  static const short lookup[] =
    {
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,   0,  -1,   1,  -1,   2,  -1,   3,  -1,   4,
       -1,   5,  -1,   6,  -1,   7,  -1,   8,   9,  10,
       11,  12,  13,  14,  15,  16,  17,  18,  19,  20,
       21,  22,  23,  24,  25,  26,  27,  28,  29,  30,
       31,  32,  33,  -1,  34,  -1,  35,  36,  37,  38,
       -1,  39,  40,  41,  -1,  42,  43,  44,  45,  -1,
       -1,  46,  47,  48,  49,  50,  51,  52,  -1,  53,
       54,  55,  56,  -1,  57,  58,  59,  60,  -1,  -1,
       -1,  -1,  61,  -1,  -1,  -1,  62,  -1,  63,  64,
       65,  66,  67,  68,  69,  -1,  -1,  70,  71,  72,
       73,  74,  75,  -1,  76,  77,  78,  79,  80,  81,
       82,  83,  -1,  84,  85,  86,  87,  88,  89,  -1,
       90,  91,  92,  93,  94,  95,  -1,  96,  97,  98,
       99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
      109, 110, 111, 112,  -1, 113, 114, 115,  -1, 116,
      117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
       -1, 127,  -1,  -1, 128, 129, 130,  -1,  -1, 131,
      132, 133, 134, 135, 136, 137,  -1, 138,  -1, 139,
      140, 141, 142,  -1, 143, 144,  -1, 145, 146, 147,
       -1,  -1, 148, 149, 150, 151, 152, 153, 154, 155,
       -1, 156,  -1, 157, 158,  -1,  -1,  -1,  -1,  -1,
       -1, 159,  -1,  -1, 160, 161, 162,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1, 163,  -1,  -1,
      164, 165,  -1, 166,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 167,  -1, 168,  -1,  -1,  -1,  -1,  -1,
       -1,  -1, 169, 170,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1, 171,  -1,  -1,  -1,  -1,  -1, 172,
       -1,  -1,  -1,  -1,  -1,  -1, 173,  -1,  -1,  -1,
       -1,  -1, 174,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 175,
       -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, 176
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
