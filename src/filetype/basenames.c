typedef struct {
    const char key[16];
    const FileTypeEnum filetype;
} FileBasenameMap;

static const FileBasenameMap basenames[] = {
    {"APKBUILD", SHELL},
    {"BSDmakefile", MAKE},
    {"BUILD.bazel", PYTHON},
    {"CMakeLists.txt", CMAKE},
    {"COMMIT_EDITMSG", GITCOMMIT},
    {"Capfile", RUBY},
    {"Cargo.lock", TOML},
    {"Dockerfile", DOCKER},
    {"Doxyfile", CONFIG},
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
    {"build.gradle", GRADLE},
    {"config.ld", LUA},
    {"configure.ac", M4},
    {"fstab", CONFIG},
    {"git-rebase-todo", GITREBASE},
    {"hosts", CONFIG},
    {"ip6tables.rules", CONFIG},
    {"iptables.rules", CONFIG},
    {"krb5.conf", INI},
    {"makefile", MAKE},
    {"menu.lst", CONFIG},
    {"meson.build", MESON},
    {"mimeapps.list", INI},
    {"mkinitcpio.conf", SHELL},
    {"nginx.conf", NGINX},
    {"pacman.conf", INI},
    {"robots.txt", ROBOTSTXT},
    {"rockspec.in", LUA},
    {"terminalrc", INI},
    {"texmf.cnf", TEXMFCNF},
    {"yum.conf", INI},
};

// These are matched with or without a leading dot
static const FileBasenameMap dotfiles[] = {
    {"Xresources", XRESOURCES},
    {"bash_logout", SHELL},
    {"bash_profile", SHELL},
    {"bashrc", SHELL},
    {"clang-format", YAML},
    {"clang-tidy", YAML},
    {"cshrc", SHELL},
    {"drirc", XML},
    {"editorconfig", INI},
    {"emacs", EMACSLISP},
    {"gemrc", YAML},
    {"gitattributes", CONFIG},
    {"gitconfig", INI},
    {"gitmodules", INI},
    {"gnus", EMACSLISP},
    {"indent.pro", INDENT},
    {"inputrc", CONFIG},
    {"jshintrc", JSON},
    {"lcovrc", CONFIG},
    {"luacheckrc", LUA},
    {"luacov", LUA},
    {"profile", SHELL},
    {"xinitrc", SHELL},
    {"xprofile", SHELL},
    {"xserverrc", SHELL},
    {"zlogin", SHELL},
    {"zlogout", SHELL},
    {"zprofile", SHELL},
    {"zshenv", SHELL},
    {"zshrc", SHELL},
};

static FileTypeEnum filetype_from_basename(StringView sv)
{
    switch (sv.length) {
    case  5:  case 6:  case 7:  case 8:
    case  9: case 10: case 11: case 12:
    case 13: case 14: case 15:
        break;
    case 17:
        return memcmp(sv.data, "meson_options.txt", 17) ? NONE : MESON;
    default:
        return NONE;
    }

    const FileBasenameMap *e = bsearch (
        &sv,
        basenames,
        ARRAY_COUNT(basenames),
        sizeof(basenames[0]),
        ft_compare
    );
    if (e) {
        return e->filetype;
    }

    if (sv.data[0] == '.') {
        sv.data++;
        sv.length--;
    }
    e = bsearch (
        &sv,
        dotfiles,
        ARRAY_COUNT(dotfiles),
        sizeof(dotfiles[0]),
        ft_compare
    );
    return e ? e->filetype : NONE;
}
