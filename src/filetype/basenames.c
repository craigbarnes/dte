static const struct FileBasenameMap {
    const char name[16];
    const uint8_t filetype; // FileTypeEnum
    bool dotfile; // If true, name is matched with or without a leading dot
} basenames[] = {
    {"APKBUILD", SH, false},
    {"BSDmakefile", MAKE, false},
    {"BUILD.bazel", PYTHON, false},
    {"CMakeLists.txt", CMAKE, false},
    {"COMMIT_EDITMSG", GITCOMMIT, false},
    {"Capfile", RUBY, false},
    {"Cargo.lock", TOML, false},
    {"Dockerfile", DOCKER, false},
    {"Doxyfile", CONFIG, false},
    {"GNUmakefile", MAKE, false},
    {"Gemfile", RUBY, false},
    {"Gemfile.lock", RUBY, false},
    {"Kbuild", MAKE, false},
    {"Makefile", MAKE, false},
    {"Makefile.am", MAKE, false},
    {"Makefile.in", MAKE, false},
    {"PKGBUILD", SH, false},
    {"Project.ede", LISP, false},
    {"Rakefile", RUBY, false},
    {"Vagrantfile", RUBY, false},
    {"XCompose", CONFIG, true},
    {"Xresources", XRESOURCES, true},
    {"bash_logout", SH, true},
    {"bash_profile", SH, true},
    {"bashrc", SH, true},
    {"build.gradle", GRADLE, false},
    {"clang-format", YAML, true},
    {"clang-tidy", YAML, true},
    {"config.ld", LUA, false},
    {"configure.ac", M4, false},
    {"cshrc", SH, true},
    {"curlrc", CONFIG, true},
    {"dircolors", CONFIG, true},
    {"drirc", XML, true},
    {"editorconfig", INI, true},
    {"emacs", LISP, true},
    {"fstab", CONFIG, false},
    {"gdbinit", CONFIG, true},
    {"gemrc", YAML, true},
    {"git-rebase-todo", GITREBASE, false},
    {"gitattributes", CONFIG, true},
    {"gitconfig", INI, true},
    {"gitmodules", INI, true},
    {"gnus", LISP, true},
    {"hosts", CONFIG, false},
    {"indent.pro", INDENT, true},
    {"inputrc", CONFIG, true},
    {"ip6tables.rules", CONFIG, false},
    {"iptables.rules", CONFIG, false},
    {"jshintrc", JSON, true},
    {"krb5.conf", INI, false},
    {"lcovrc", CONFIG, true},
    {"lesskey", CONFIG, true},
    {"luacheckrc", LUA, true},
    {"luacov", LUA, true},
    {"makefile", MAKE, false},
    {"menu.lst", CONFIG, false},
    {"meson.build", MESON, false},
    {"mimeapps.list", INI, false},
    {"mkinitcpio.conf", SH, false},
    {"muttrc", CONFIG, true},
    {"nanorc", CONFIG, true},
    {"nginx.conf", NGINX, false},
    {"pacman.conf", INI, false},
    {"profile", SH, true},
    {"robots.txt", ROBOTSTXT, false},
    {"rockspec.in", LUA, false},
    {"shellcheckrc", CONFIG, true},
    {"sxhkdrc", CONFIG, true},
    {"terminalrc", INI, false},
    {"texmf.cnf", TEXMFCNF, false},
    {"tigrc", CONFIG, true},
    {"tmux.conf", TMUX, true},
    {"xinitrc", SH, true},
    {"xprofile", SH, true},
    {"xserverrc", SH, true},
    {"yum.conf", INI, false},
    {"zlogin", SH, true},
    {"zlogout", SH, true},
    {"zprofile", SH, true},
    {"zshenv", SH, true},
    {"zshrc", SH, true},
};

static FileTypeEnum filetype_from_basename(StringView name)
{
    if (name.length < 4) {
        return NONE;
    }

    if (name.length >= ARRAY_COUNT(basenames[0].name)) {
        if (strview_equal_cstring(&name, "meson_options.txt")) {
            return MESON;
        }
        return NONE;
    }

    bool dot = (name.data[0] == '.');
    if (dot) {
        strview_remove_prefix(&name, 1);
    }

    const struct FileBasenameMap *e = BSEARCH(&name, basenames, ft_compare);
    if (e && (!dot || e->dotfile)) {
        return e->filetype;
    }

    return NONE;
}
