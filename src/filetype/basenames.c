/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 -n src/filetype/basenames.gperf  */
/* Computed positions: -k'1-2,10' */
/* Filtered by: tools/gperf-filter.sed */
/* maximum key range = 43, duplicates = 0 */

inline
static unsigned int
ft_base_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43,  4, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 23,  0, 29, 23, 43,
      43, 25, 43,  6, 43, 20, 43,  6, 10,  6,
      19, 43, 26, 33, 43,  0, 21, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 11, 43,  9,
      16,  1, 14,  0, 43,  2, 29, 16,  3,  5,
      20,  6,  0, 43,  5, 43, 13,  0, 43, 43,
      43,  0, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43, 43, 43, 43, 43,
      43, 43, 43, 43, 43, 43
    };
  register unsigned int hval = 0;

  switch (len)
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
      TOTAL_KEYWORDS = 43,
      MIN_WORD_LENGTH = 5,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 0,
      MAX_HASH_VALUE = 42
    };

  static const unsigned char lengthtable[] =
    {
       8, 11,  9, 15, 10, 11,  8,  7, 17, 11, 13, 11, 11, 11,
       9,  9,  8,  8, 13, 11,  6,  5,  7, 10, 10, 13,  7, 15,
      11,  8, 10, 11, 12,  9, 10, 11, 11,  8, 11,  8,  7, 14,
       8
    };
  static const FileTypeHashSlot wordlist[] =
    {
      {"yum.conf", INI},
      {"BUILD.bazel", PYTHON},
      {"gitconfig", INI},
      {"git-rebase-todo", GITREBASE},
      {".gitconfig", INI},
      {".gitmodules", INI},
      {".inputrc", INPUTRC},
      {".luacov", LUA},
      {"meson_options.txt", MESON},
      {"meson.build", MESON},
      {"mimeapps.list", INI},
      {".indent.pro", INDENT},
      {".luacheckrc", LUA},
      {"rockspec.in", LUA},
      {"texmf.cnf", TEXMFCNF},
      {"config.ld", LUA},
      {"makefile", MAKE},
      {"Makefile", MAKE},
      {".clang-format", YAML},
      {"Makefile.in", MAKE},
      {".drirc", XML},
      {"drirc", XML},
      {"inputrc", INPUTRC},
      {"terminalrc", INI},
      {"robots.txt", ROBOTSTXT},
      {".editorconfig", INI},
      {"Gemfile", RUBY},
      {"mkinitcpio.conf", SHELL},
      {"Makefile.am", MAKE},
      {"Doxyfile", DOXYGEN},
      {"Dockerfile", DOCKER},
      {"pacman.conf", INI},
      {"Gemfile.lock", RUBY},
      {".jshintrc", JSON},
      {"nginx.conf", NGINX},
      {"Vagrantfile", RUBY},
      {"BSDmakefile", MAKE},
      {"Rakefile", RUBY},
      {"GNUmakefile", MAKE},
      {"PKGBUILD", SHELL},
      {"Capfile", RUBY},
      {"COMMIT_EDITMSG", GITCOMMIT},
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
