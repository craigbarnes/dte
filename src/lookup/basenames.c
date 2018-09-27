static const struct FileBasenameMap {
    const char key[16];
    const FileTypeEnum filetype;
} basenames[] = {
    {".bash_logout", SHELL},
    {".bash_profile", SHELL},
    {".bashrc", SHELL},
    {".clang-format", YAML},
    {".cshrc", SHELL},
    {".drirc", XML},
    {".editorconfig", INI},
    {".emacs", EMACSLISP},
    {".gemrc", YAML},
    {".gitattributes", CONFIG},
    {".gitconfig", INI},
    {".gitmodules", INI},
    {".gnus", EMACSLISP},
    {".indent.pro", INDENT},
    {".inputrc", INPUTRC},
    {".jshintrc", JSON},
    {".luacheckrc", LUA},
    {".luacov", LUA},
    {".profile", SHELL},
    {".zlogin", SHELL},
    {".zlogout", SHELL},
    {".zprofile", SHELL},
    {".zshenv", SHELL},
    {".zshrc", SHELL},
    {"APKBUILD", SHELL},
    {"BSDmakefile", MAKE},
    {"BUILD.bazel", PYTHON},
    {"CMakeLists.txt", CMAKE},
    {"COMMIT_EDITMSG", GITCOMMIT},
    {"Capfile", RUBY},
    {"Cargo.lock", TOML},
    {"Dockerfile", DOCKER},
    {"Doxyfile", DOXYGEN},
    {"GNUmakefile", MAKE},
    {"Gemfile", RUBY},
    {"Gemfile.lock", RUBY},
    {"Kbuild", MAKE},
    {"Makefile", MAKE},
    {"Makefile.am", MAKE},
    {"Makefile.in", MAKE},
    {"PKGBUILD", SHELL},
    {"Project.ede", EMACSLISP},
    {"Rakefile", RUBY},
    {"Vagrantfile", RUBY},
    {"bash_logout", SHELL},
    {"bash_profile", SHELL},
    {"bashrc", SHELL},
    {"config.ld", LUA},
    {"configure.ac", M4},
    {"cshrc", SHELL},
    {"drirc", XML},
    {"git-rebase-todo", GITREBASE},
    {"gitattributes", CONFIG},
    {"gitconfig", INI},
    {"inputrc", INPUTRC},
    {"krb5.conf", INI},
    {"makefile", MAKE},
    {"meson.build", MESON},
    {"mimeapps.list", INI},
    {"mkinitcpio.conf", SHELL},
    {"nginx.conf", NGINX},
    {"pacman.conf", INI},
    {"profile", SHELL},
    {"robots.txt", ROBOTSTXT},
    {"rockspec.in", LUA},
    {"terminalrc", INI},
    {"texmf.cnf", TEXMFCNF},
    {"yum.conf", INI},
    {"zlogin", SHELL},
    {"zlogout", SHELL},
    {"zprofile", SHELL},
    {"zshenv", SHELL},
    {"zshrc", SHELL},
};

static int basename_cmp(const void *key, const void *elem)
{
    const StringView *sv = key;
    const char *ext = elem; // Cast to first member of struct
    return memcmp(sv->data, ext, sv->length);
}

static FileTypeEnum filetype_from_basename(const char *s, size_t len)
{
    switch (len) {
    case  5:  case 6:  case 7:  case 8:
    case  9: case 10: case 11: case 12:
    case 13: case 14: case 15: case 16:
        break;
    case 17:
        return memcmp(s, "meson_options.txt", len) ? NONE : MESON;
    default:
        return NONE;
    }

    const StringView sv = string_view(s, len);
    const struct FileBasenameMap *e = bsearch (
        &sv,
        basenames,
        ARRAY_COUNT(basenames),
        sizeof(basenames[0]),
        basename_cmp
    );
    return e ? e->filetype : 0;
}
