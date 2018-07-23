/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -m50 src/lookup/basenames.gperf  */
/* Computed positions: -k'1-2,$' */
/* Filtered by: mk/gperf-filter.sed */
/* maximum key range = 92, duplicates = 0 */

inline
static unsigned int
ft_basename_hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99,  8, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 50, 11, 35,  1, 99,
      99,  0, 99, 99, 99, 37, 99, 27,  2, 22,
      13, 99, 47, 44, 99, 11, 41, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 12, 11,  0,
      29,  0, 46,  2, 99, 23,  3,  0, 32, 13,
      11,  0, 29, 99, 34,  3,  0,  4, 26, 99,
      99,  4, 11, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99
    };
  return len + asso_values[(unsigned char)str[1]] + asso_values[(unsigned char)str[0]] + asso_values[(unsigned char)str[len - 1]];
}

static const FileTypeHashSlot*
filetype_from_basename (register const char *str, register size_t len)
{
  enum
    {
      TOTAL_KEYWORDS = 71,
      MIN_WORD_LENGTH = 5,
      MAX_WORD_LENGTH = 17,
      MIN_HASH_VALUE = 7,
      MAX_HASH_VALUE = 98
    };

  static const unsigned char lengthtable[] =
    {
       0,  0,  0,  0,  0,  0,  0,  7,  5,  8, 10, 10, 12, 11,
       6,  0,  6,  6,  5,  5,  9, 13, 10, 13, 11,  6,  7,  8,
       9,  6, 17, 12, 13,  8, 11, 12,  9,  7,  9,  8, 15,  7,
      11,  6, 10,  8,  6,  8,  8, 13,  7, 11,  7, 11,  7,  9,
      11, 10, 11,  8,  6, 11,  8, 11, 11, 11, 11,  8,  5, 10,
       7, 14,  8,  7, 15,  0, 14,  0,  0,  0,  0,  0,  0,  6,
       0,  0,  0,  0,  0,  9,  0,  0,  0,  0,  0,  0,  0,  0,
      11
    };
  static const FileTypeHashSlot ft_basename_table[] =
    {
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"Gemfile", RUBY},
      {"cshrc", SHELL},
      {"Doxyfile", DOXYGEN},
      {"terminalrc", INI},
      {"Dockerfile", DOCKER},
      {"Gemfile.lock", RUBY},
      {"GNUmakefile", MAKE},
      {".cshrc", SHELL},
      {"",0},
      {".gemrc", YAML},
      {".emacs", EMACSLISP},
      {".gnus", EMACSLISP},
      {"zshrc", SHELL},
      {".jshintrc", JSON},
      {".clang-format", YAML},
      {".gitconfig", INI},
      {".editorconfig", INI},
      {".gitmodules", INI},
      {".zshrc", SHELL},
      {".bashrc", SHELL},
      {".zlogout", SHELL},
      {".zprofile", SHELL},
      {"bashrc", SHELL},
      {"meson_options.txt", MESON},
      {".bash_logout", SHELL},
      {".bash_profile", SHELL},
      {"makefile", MAKE},
      {"bash_logout", SHELL},
      {"bash_profile", SHELL},
      {"gitconfig", INI},
      {".zlogin", SHELL},
      {"config.ld", LUA},
      {".inputrc", INPUTRC},
      {"git-rebase-todo", GITREBASE},
      {"inputrc", INPUTRC},
      {".indent.pro", INDENT},
      {".drirc", XML},
      {"robots.txt", ROBOTSTXT},
      {".profile", SHELL},
      {"zshenv", SHELL},
      {"Makefile", MAKE},
      {"zprofile", SHELL},
      {"mimeapps.list", INI},
      {"zlogout", SHELL},
      {".luacheckrc", LUA},
      {".zshenv", SHELL},
      {"meson.build", MESON},
      {"Capfile", RUBY},
      {"texmf.cnf", TEXMFCNF},
      {"rockspec.in", LUA},
      {"Cargo.lock", TOML},
      {"Project.ede", EMACSLISP},
      {"PKGBUILD", SHELL},
      {"zlogin", SHELL},
      {"Makefile.in", MAKE},
      {"yum.conf", INI},
      {"Makefile.am", MAKE},
      {"Vagrantfile", RUBY},
      {"BUILD.bazel", PYTHON},
      {"BSDmakefile", MAKE},
      {"Rakefile", RUBY},
      {"drirc", XML},
      {"nginx.conf", NGINX},
      {"profile", SHELL},
      {"COMMIT_EDITMSG", GITCOMMIT},
      {"APKBUILD", SHELL},
      {".luacov", LUA},
      {"mkinitcpio.conf", SHELL},
      {"",0},
      {"CMakeLists.txt", CMAKE},
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"Kbuild", MAKE},
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"krb5.conf", INI},
      {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0}, {"",0},
      {"",0},
      {"pacman.conf", INI}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = ft_basename_hash (str, len);

      if (key <= MAX_HASH_VALUE)
        if (len == lengthtable[key])
          {
            register const char *s = ft_basename_table[key].key;

            if (*str == *s && !memcmp (str + 1, s + 1, len - 1))
              return &ft_basename_table[key];
          }
    }
  return 0;
}
