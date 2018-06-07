/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/filetype/basenames.gperf  */
/* Computed positions: -k'1,11' */
/* Filtered by: tools/gperf-filter.sed */

#define BASENAMES_TOTAL_KEYWORDS 43
#define BASENAMES_MIN_WORD_LENGTH 5
#define BASENAMES_MAX_WORD_LENGTH 17
#define BASENAMES_MIN_HASH_VALUE 7
#define BASENAMES_MAX_HASH_VALUE 49
/* maximum key range = 43, duplicates = 0 */

inline
static unsigned int
ft_base_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50,  1,  3, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 40, 24,  0, 26, 50,
      50, 23, 50, 50, 50, 50, 50,  6, 50, 50,
      39, 50, 38, 50, 30, 50, 27, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 10,
      22,  5, 21, 33, 50, 19, 50, 50,  6,  0,
       5, 11,  6, 50, 13, 21, 11, 50, 50, 50,
      50, 23, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
      50, 50, 50, 50, 50, 50
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
      case 9:
      case 8:
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const FileTypeHashSlot*
filetype_from_basename (register const char *str, register size_t len)
{
  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  0,  7,  8,  6,  7,  8,  9, 10,
       8, 10, 13, 11, 15,  9,  9, 10, 11, 10, 11, 11,  7,  5,
      17, 11,  7,  8, 13, 11,  8, 11, 10, 13, 11, 11, 11, 11,
       9, 11, 14, 12,  8,  8,  8, 15
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"Capfile", RUBY},
      {"makefile", MAKE},
      {".drirc", XML},
      {".luacov", LUA},
      {".inputrc", INPUTRC},
      {".jshintrc", JSON},
      {".gitconfig", INI},
      {"Makefile", MAKE},
      {"nginx.conf", NGINX},
      {".clang-format", YAML},
      {"Makefile.am", MAKE},
      {"mkinitcpio.conf", SHELL},
      {"config.ld", LUA},
      {"texmf.cnf", TEXMFCNF},
      {"terminalrc", INI},
      {"Makefile.in", MAKE},
      {"robots.txt", ROBOTSTXT},
      {".luacheckrc", LUA},
      {".indent.pro", INDENT},
      {"inputrc", INPUTRC},
      {"drirc", XML},
      {"meson_options.txt", MESON},
      {"rockspec.in", LUA},
      {"Gemfile", RUBY},
      {"yum.conf", INI},
      {"mimeapps.list", INI},
      {"meson.build", MESON},
      {"Doxyfile", DOXYGEN},
      {".gitmodules", INI},
      {"Dockerfile", DOCKER},
      {".editorconfig", INI},
      {"pacman.conf", INI},
      {"GNUmakefile", MAKE},
      {"BSDmakefile", MAKE},
      {"BUILD.bazel", PYTHON},
      {"gitconfig", INI},
      {"Vagrantfile", RUBY},
      {"COMMIT_EDITMSG", GITCOMMIT},
      {"Gemfile.lock", RUBY},
      {"Rakefile", RUBY},
      {"PKGBUILD", SHELL},
      {"APKBUILD", SHELL},
      {"git-rebase-todo", GITREBASE}
    };

  if (len <= BASENAMES_MAX_WORD_LENGTH && len >= BASENAMES_MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_base_hash (str, len);

      if (key <= BASENAMES_MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].key;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
