/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/lookup/basenames.gperf  */
/* Computed positions: -k'1-2,10' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 50, duplicates = 0 */

inline
static unsigned int
ft_base_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56,  0, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 40, 19, 16, 21, 56,
      56, 27, 56, 12, 56, 28, 56,  3, 11, 11,
       7, 56, 28, 17, 56, 19, 19, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 15, 13,  8,
      21,  0,  9,  2, 56,  1, 31, 13,  3,  2,
      20,  6,  0, 56,  3,  6, 13,  2, 56, 56,
      56,  1, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
      56, 56, 56, 56, 56, 56
    };
  register unsigned int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
      case 8:
      case 7:
      case 6:
      case 5:
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const FileTypeHashSlot*
filetype_from_basename (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 50,
      MIN_WORD_LENGTH = 5,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 6,
      MAX_HASH_VALUE = 55
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  6,  5,  6,  8,  7,  8,  9, 11,
      10, 11, 11, 11, 15, 13, 17, 11,  9,  9, 13,  8,  8,  6,
       7,  5, 11, 10, 10, 13,  7,  8, 15, 10,  7, 14,  9, 10,
      11,  8, 11, 12, 11,  6, 11, 11, 11,  8, 11, 14, 10,  8
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {".emacs", EMACSLISP},
      {".gnus", EMACSLISP},
      {".gemrc", YAML},
      {".inputrc", INPUTRC},
      {".luacov", LUA},
      {"yum.conf", INI},
      {"gitconfig", INI},
      {".gitmodules", INI},
      {".gitconfig", INI},
      {".indent.pro", INDENT},
      {"meson.build", MESON},
      {".luacheckrc", LUA},
      {"git-rebase-todo", GITREBASE},
      {"mimeapps.list", INI},
      {"meson_options.txt", MESON},
      {"rockspec.in", LUA},
      {"texmf.cnf", TEXMFCNF},
      {"config.ld", LUA},
      {".clang-format", YAML},
      {"makefile", MAKE},
      {"Makefile", MAKE},
      {".drirc", XML},
      {"inputrc", INPUTRC},
      {"drirc", XML},
      {"Makefile.in", MAKE},
      {"terminalrc", INI},
      {"robots.txt", ROBOTSTXT},
      {".editorconfig", INI},
      {"Gemfile", RUBY},
      {"Doxyfile", DOXYGEN},
      {"mkinitcpio.conf", SHELL},
      {"Dockerfile", DOCKER},
      {"Capfile", RUBY},
      {"CMakeLists.txt", CMAKE},
      {".jshintrc", JSON},
      {"nginx.conf", NGINX},
      {"Project.ede", EMACSLISP},
      {"PKGBUILD", SHELL},
      {"Makefile.am", MAKE},
      {"Gemfile.lock", RUBY},
      {"pacman.conf", INI},
      {"Kbuild", MAKE},
      {"Vagrantfile", RUBY},
      {"BUILD.bazel", PYTHON},
      {"BSDmakefile", MAKE},
      {"Rakefile", RUBY},
      {"GNUmakefile", MAKE},
      {"COMMIT_EDITMSG", GITCOMMIT},
      {"Cargo.lock", TOML},
      {"APKBUILD", SHELL}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_base_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = wordlist[key].key;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &wordlist[key];
          }
    }
  return 0;
}
