/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/lookup/basenames.gperf  */
/* Computed positions: -k'1-2,$' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 95, duplicates = 0 */

inline
static unsigned int
filetype_basename_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102,   5, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102,   0,  16,  31,  46, 102,
      102,  56, 102, 102, 102,  21, 102,  29,   8,   0,
       23, 102,  49,  46, 102,  15,  43, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102,  17,   8,   1,
       22,   0,  26,   0, 102,  15,   8,   2,  30,  10,
       25,   8,  36, 102,  22,   0,   6,  17,  16, 102,
      102,  17,   8, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102, 102, 102, 102, 102,
      102, 102, 102, 102, 102, 102
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]] + asso_values[(unsigned char)str[len - 1]];
}

static const FileTypeHashSlot*
filetype_from_basename (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 73,
      MIN_WORD_LENGTH = 5,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 7,
      MAX_HASH_VALUE = 101
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  5,  6,  6,  6,
       5, 10, 11, 10, 13, 14,  6,  7,  9,  9,  9, 13, 13,  8,
      13,  8,  6, 12,  6, 17,  6,  8,  7, 12, 15, 11,  9,  9,
      11, 11, 13,  7, 10, 11,  7,  8,  5,  7,  8, 15,  8,  7,
      11,  6,  7,  9, 10, 10,  8,  7, 10,  7, 11, 11,  8,  6,
      12, 11, 11, 11,  8, 11,  0,  8,  0,  0, 14,  0, 11,  0,
       0,  0,  0,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,
       8,  0,  0, 14
    };
  static const FileTypeHashSlot filetype_basename_table[] =
    {
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"cshrc", SHELL},
      {"",0}, {"",0},
      {".gnus", EMACSLISP},
      {".emacs", EMACSLISP},
      {".gemrc", YAML},
      {".cshrc", SHELL},
      {"zshrc", SHELL},
      {".gitconfig", INI},
      {".gitmodules", INI},
      {"terminalrc", INI},
      {".editorconfig", INI},
      {".gitattributes", CONFIG},
      {".zshrc", SHELL},
      {".bashrc", SHELL},
      {".zprofile", SHELL},
      {".jshintrc", JSON},
      {"gitconfig", INI},
      {".clang-format", YAML},
      {".bash_profile", SHELL},
      {".zlogout", SHELL},
      {"gitattributes", CONFIG},
      {".inputrc", INPUTRC},
      {"zshenv", SHELL},
      {".bash_logout", SHELL},
      {"bashrc", SHELL},
      {"meson_options.txt", MESON},
      {".drirc", XML},
      {"makefile", MAKE},
      {".zshenv", SHELL},
      {"bash_profile", SHELL},
      {"git-rebase-todo", GITREBASE},
      {".indent.pro", INDENT},
      {"config.ld", LUA},
      {"texmf.cnf", TEXMFCNF},
      {"bash_logout", SHELL},
      {"meson.build", MESON},
      {"mimeapps.list", INI},
      {".zlogin", SHELL},
      {"robots.txt", ROBOTSTXT},
      {".luacheckrc", LUA},
      {"inputrc", INPUTRC},
      {".profile", SHELL},
      {"drirc", XML},
      {"zlogout", SHELL},
      {"zprofile", SHELL},
      {"mkinitcpio.conf", SHELL},
      {"Makefile", MAKE},
      {"Capfile", RUBY},
      {"Project.ede", EMACSLISP},
      {"Kbuild", MAKE},
      {".luacov", LUA},
      {"krb5.conf", INI},
      {"Cargo.lock", TOML},
      {"nginx.conf", NGINX},
      {"Doxyfile", DOXYGEN},
      {"Gemfile", RUBY},
      {"Dockerfile", DOCKER},
      {"profile", SHELL},
      {"rockspec.in", LUA},
      {"Makefile.am", MAKE},
      {"yum.conf", INI},
      {"zlogin", SHELL},
      {"Gemfile.lock", RUBY},
      {"Vagrantfile", RUBY},
      {"BUILD.bazel", PYTHON},
      {"BSDmakefile", MAKE},
      {"Rakefile", RUBY},
      {"GNUmakefile", MAKE},
      {"",0},
      {"APKBUILD", SHELL},
      {"",0}, {"",0},
      {"CMakeLists.txt", CMAKE},
      {"",0},
      {"Makefile.in", MAKE},
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"pacman.conf", INI},
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"PKGBUILD", SHELL},
      {"",0}, {"",0},
      {"COMMIT_EDITMSG", GITCOMMIT}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = filetype_basename_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = filetype_basename_table[key].key;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &filetype_basename_table[key];
          }
    }
  return 0;
}
