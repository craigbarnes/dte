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
    {"Project.ede", LISP},
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
    {"XCompose", CONFIG},
    {"Xresources", XRESOURCES},
    {"bash_logout", SHELL},
    {"bash_profile", SHELL},
    {"bashrc", SHELL},
    {"clang-format", YAML},
    {"clang-tidy", YAML},
    {"cshrc", SHELL},
    {"curlrc", CONFIG},
    {"dircolors", CONFIG},
    {"drirc", XML},
    {"editorconfig", INI},
    {"emacs", LISP},
    {"gdbinit", CONFIG},
    {"gemrc", YAML},
    {"gitattributes", CONFIG},
    {"gitconfig", INI},
    {"gitmodules", INI},
    {"gnus", LISP},
    {"indent.pro", INDENT},
    {"inputrc", CONFIG},
    {"jshintrc", JSON},
    {"lcovrc", CONFIG},
    {"lesskey", CONFIG},
    {"luacheckrc", LUA},
    {"luacov", LUA},
    {"muttrc", CONFIG},
    {"nanorc", CONFIG},
    {"profile", SHELL},
    {"shellcheckrc", CONFIG},
    {"sxhkdrc", CONFIG},
    {"tigrc", CONFIG},
    {"tmux.conf", TMUX},
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
    if (sv.length < 4) {
        return NONE;
    } else if (sv.length >= ARRAY_COUNT(basenames[0].key)) {
        if (strview_equal_cstring(&sv, "meson_options.txt")) {
            return MESON;
        }
        return NONE;
    }

    const FileBasenameMap *e = BSEARCH(&sv, basenames, ft_compare);
    if (e) {
        return e->filetype;
    }

    if (sv.data[0] == '.') {
        sv.data++;
        sv.length--;
    }
    e = BSEARCH(&sv, dotfiles, ft_compare);
    return e ? e->filetype : NONE;
}
