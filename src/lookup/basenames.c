static const struct {
    const char *const key;
    const FileTypeEnum val;
} filetype_from_basename_table[74] = {
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
    {"meson_options.txt", MESON},
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

#define CMP(i) idx = i; goto compare
#define CMPN(i) idx = i; goto compare_last_char
#define KEY filetype_from_basename_table[idx].key
#define VAL filetype_from_basename_table[idx].val

static FileTypeEnum filetype_from_basename(const char *s, size_t len)
{
    size_t idx;
    switch (len) {
    case 5:
        switch (s[0]) {
        case '.': CMP(12); // .gnus
        case 'c': CMP(49); // cshrc
        case 'd': CMP(50); // drirc
        case 'z': CMP(73); // zshrc
        }
        break;
    case 6:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'c': CMP(4); // .cshrc
            case 'd': CMP(5); // .drirc
            case 'e': CMP(7); // .emacs
            case 'g': CMP(8); // .gemrc
            case 'z': CMP(23); // .zshrc
            }
            break;
        case 'K': CMP(36); // Kbuild
        case 'b': CMP(46); // bashrc
        case 'z':
            switch (s[1]) {
            case 'l': CMP(69); // zlogin
            case 's': CMP(72); // zshenv
            }
            break;
        }
        break;
    case 7:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'b': CMP(2); // .bashrc
            case 'l': CMP(17); // .luacov
            case 'z':
                switch (s[2]) {
                case 'l': CMP(19); // .zlogin
                case 's': CMP(22); // .zshenv
                }
                break;
            }
            break;
        case 'C': CMP(29); // Capfile
        case 'G': CMP(34); // Gemfile
        case 'i': CMP(54); // inputrc
        case 'p': CMP(63); // profile
        case 'z': CMP(70); // zlogout
        }
        break;
    case 8:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'i': CMP(14); // .inputrc
            case 'p': CMP(18); // .profile
            case 'z': CMP(20); // .zlogout
            }
            break;
        case 'A': CMP(24); // APKBUILD
        case 'D': CMP(32); // Doxyfile
        case 'M': CMP(37); // Makefile
        case 'P': CMP(40); // PKGBUILD
        case 'R': CMP(42); // Rakefile
        case 'm': CMP(56); // makefile
        case 'y': CMP(68); // yum.conf
        case 'z': CMP(71); // zprofile
        }
        break;
    case 9:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'j': CMP(15); // .jshintrc
            case 'z': CMP(21); // .zprofile
            }
            break;
        case 'c': CMP(47); // config.ld
        case 'g': CMP(53); // gitconfig
        case 'k': CMP(55); // krb5.conf
        case 't': CMP(67); // texmf.cnf
        }
        break;
    case 10:
        switch (s[0]) {
        case '.': CMP(10); // .gitconfig
        case 'C': CMP(30); // Cargo.lock
        case 'D': CMP(31); // Dockerfile
        case 'n': CMP(61); // nginx.conf
        case 'r': CMP(64); // robots.txt
        case 't': CMP(66); // terminalrc
        }
        break;
    case 11:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'g': CMP(11); // .gitmodules
            case 'i': CMP(13); // .indent.pro
            case 'l': CMP(16); // .luacheckrc
            }
            break;
        case 'B':
            switch (s[1]) {
            case 'S': CMP(25); // BSDmakefile
            case 'U': CMP(26); // BUILD.bazel
            }
            break;
        case 'G': CMP(33); // GNUmakefile
        case 'M':
            switch (s[9]) {
            case 'a': CMP(38); // Makefile.am
            case 'i': CMP(39); // Makefile.in
            }
            break;
        case 'P': CMP(41); // Project.ede
        case 'V': CMP(43); // Vagrantfile
        case 'b': CMP(44); // bash_logout
        case 'm': CMP(57); // meson.build
        case 'p': CMP(62); // pacman.conf
        case 'r': CMP(65); // rockspec.in
        }
        break;
    case 12:
        switch (s[0]) {
        case '.': CMP(0); // .bash_logout
        case 'G': CMP(35); // Gemfile.lock
        case 'b': CMP(45); // bash_profile
        case 'c': CMP(48); // configure.ac
        }
        break;
    case 13:
        switch (s[0]) {
        case '.':
            switch (s[1]) {
            case 'b': CMP(1); // .bash_profile
            case 'c': CMP(3); // .clang-format
            case 'e': CMP(6); // .editorconfig
            }
            break;
        case 'g': CMP(52); // gitattributes
        case 'm': CMP(59); // mimeapps.list
        }
        break;
    case 14:
        switch (s[0]) {
        case '.': CMP(9); // .gitattributes
        case 'C':
            switch (s[1]) {
            case 'M': CMP(27); // CMakeLists.txt
            case 'O': CMP(28); // COMMIT_EDITMSG
            }
            break;
        }
        break;
    case 15:
        switch (s[0]) {
        case 'g': CMP(51); // git-rebase-todo
        case 'm': CMP(60); // mkinitcpio.conf
        }
        break;
    case 17: CMP(58); // meson_options.txt
    }
    return 0;
compare:
    return (memcmp(s, KEY, len) == 0) ? VAL : 0;
compare_last_char:
    return (s[len - 1] == KEY[len - 1]) ? VAL : 0;
}

#undef CMP
#undef CMPN
#undef KEY
#undef VAL
