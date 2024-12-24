static const struct FileBasenameMap {
    const char name[16];
    uint8_t filetype; // FileTypeEnum
    bool dotfile; // If true, name is matched with or without a leading dot
} basenames[] = {
    {"APKBUILD", SH, false},
    {"BSDmakefile", MAKE, false},
    {"BUILD.bazel", PYTHON, false},
    {"Brewfile", RUBY, true},
    {"CMakeLists.txt", CMAKE, false},
    {"COMMIT_EDITMSG", GITCOMMIT, false},
    {"Capfile", RUBY, false},
    {"Cargo.lock", TOML, false},
    {"DIR_COLORS", CONFIG, false},
    {"Dangerfile", RUBY, false},
    {"Dockerfile", DOCKER, false},
    {"Doxyfile", CONFIG, false},
    {"GNUmakefile", MAKE, false},
    {"Gemfile", RUBY, false},
    {"Gemfile.lock", RUBY, false},
    {"Guardfile", RUBY, true},
    {"Kbuild", MAKE, false},
    {"Kconfig", CONFIG, false},
    {"Kconfig.debug", CONFIG, false},
    {"MERGE_MSG", GITCOMMIT, false},
    {"Makefile", MAKE, false},
    {"Makefile.am", MAKE, false},
    {"Makefile.in", MAKE, false},
    {"NOTES_EDITMSG", GITNOTE, false},
    {"PKGBUILD", SH, false},
    {"Pipfile", TOML, false},
    {"Pipfile.lock", JSON, false},
    {"Project.ede", LISP, false},
    {"Puppetfile", RUBY, false},
    {"Rakefile", RUBY, false},
    {"Steepfile", RUBY, false},
    {"Thorfile", RUBY, false},
    {"Vagrantfile", RUBY, false},
    {"XCompose", CONFIG, true},
    {"Xresources", XRESOURCES, true},
    {"ackrc", CONFIG, true},
    {"bash_aliases", SH, true},
    {"bash_functions", SH, true},
    {"bash_logout", SH, true},
    {"bash_profile", SH, true},
    {"bashrc", SH, true},
    {"bazelrc", CONFIG, true},
    {"build.gradle", GRADLE, false},
    {"buildozer.spec", INI, false},
    {"clang-format", YAML, true},
    {"clang-tidy", YAML, true},
    {"colordiffrc", CONFIG, true},
    {"composer.lock", JSON, false},
    {"config.ld", LUA, false},
    {"configure.ac", M4, false},
    {"coveragerc", INI, true},
    {"csh.login", CSH, true},
    {"csh.logout", CSH, true},
    {"cshdirs", CSH, true},
    {"cshrc", CSH, true},
    {"curlrc", CONFIG, true},
    {"deno.lock", JSON, false},
    {"dir_colors", CONFIG, true},
    {"dircolors", CONFIG, true},
    {"doas.conf", CONFIG, false},
    {"drirc", XML, true},
    {"dterc", DTE, true},
    {"editorconfig", INI, true},
    {"emacs", LISP, true},
    {"flake.lock", JSON, false},
    {"flake8", INI, true},
    {"fstab", CONFIG, false},
    {"gdbinit", CONFIG, true},
    {"gemrc", YAML, true},
    {"git-rebase-todo", GITREBASE, false},
    {"gitattributes", CONFIG, true},
    {"gitconfig", INI, true},
    {"gitignore", GITIGNORE, true},
    {"gitmodules", INI, true},
    {"gnus", LISP, true},
    {"go.mod", GOMODULE, false},
    {"hosts", CONFIG, false},
    {"htmlhintrc", JSON, true},
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
    {"mcmod.info", JSON, false},
    {"menu.lst", CONFIG, false},
    {"meson.build", MESON, false},
    {"mimeapps.list", INI, false},
    {"mkinitcpio.conf", SH, false},
    {"muttrc", CONFIG, true},
    {"nanorc", CONFIG, true},
    {"nftables.conf", NFTABLES, false},
    {"nginx.conf", NGINX, false},
    {"pacman.conf", INI, false},
    {"pdm.lock", TOML, false},
    {"poetry.lock", TOML, false},
    {"profile", SH, true},
    {"pylintrc", INI, true},
    {"robots.txt", ROBOTSTXT, false},
    {"rockspec.in", LUA, false},
    {"shellcheckrc", CONFIG, true},
    {"shrc", SH, true},
    {"sudoers", CONFIG, false},
    {"sxhkdrc", CONFIG, true},
    {"tags", CTAGS, false},
    {"tcshrc", CSH, true},
    {"terminalrc", INI, false},
    {"terminfo.src", TERMINFO, false},
    {"texmf.cnf", TEXMFCNF, false},
    {"tigrc", CONFIG, true},
    {"tmux.conf", TMUX, true},
    {"user-dirs.conf", CONFIG, false},
    {"user-dirs.dirs", CONFIG, false},
    {"vconsole.conf", CONFIG, false},
    {"watchmanconfig", JSON, true},
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
    size_t dotprefix = 0;
    if (strview_has_prefix(&name, ".")) {
        dotprefix = 1;
    } else if (strview_has_prefix(&name, "dot_")) {
        dotprefix = 4;
    }

    strview_remove_prefix(&name, dotprefix);
    if (name.length < 4) {
        return NONE;
    }

    if (name.length >= ARRAYLEN(basenames[0].name)) {
        if (strview_equal_cstring(&name, "meson_options.txt")) {
            return dotprefix ? NONE : MESON;
        } else if (strview_equal_cstring(&name, "git-blame-ignore-revs")) {
            return dotprefix ? CONFIG : NONE;
        }
        return NONE;
    }

    const struct FileBasenameMap *e = BSEARCH(&name, basenames, ft_compare);
    return (e && (dotprefix == 0 || e->dotfile)) ? e->filetype : NONE;
}
