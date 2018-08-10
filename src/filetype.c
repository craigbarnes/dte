#include "filetype.h"
#include "common.h"
#include "regexp.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/xmalloc.h"

typedef enum {
    NONE = 0,
    ADA,
    ASSEMBLY,
    AWK,
    BATCHFILE,
    BIBTEX,
    C,
    CLOJURE,
    CMAKE,
    COFFEESCRIPT,
    COMMONLISP,
    CONFIG,
    CPLUSPLUS,
    CSHARP,
    CSS,
    CSV,
    D,
    DART,
    DIFF,
    DOCKER,
    DOT,
    DOXYGEN,
    DTERC,
    EMACSLISP,
    EMAIL,
    GETTEXT,
    GITCOMMIT,
    GITREBASE,
    GLSL,
    GNUPLOT,
    GO,
    GPERF,
    GROOVY,
    HASKELL,
    HTML,
    INDENT,
    INI,
    INPUTRC,
    JAVA,
    JAVASCRIPT,
    JSON,
    LEX,
    LUA,
    M4,
    MAKE,
    MALLARD,
    MARKDOWN,
    MESON,
    MOONSCRIPT,
    NGINX,
    NIM,
    NINJA,
    NIX,
    OBJECTIVEC,
    PERL,
    PHP,
    PKGCONFIG,
    PROTOBUF,
    PYTHON,
    RACKET,
    ROBOTSTXT,
    ROFF,
    RUBY,
    RUST,
    SCALA,
    SCHEME,
    SCSS,
    SED,
    SHELL,
    SQL,
    TCL,
    TEX,
    TEXINFO,
    TEXMFCNF,
    TOML,
    TYPESCRIPT,
    VALA,
    VCARD,
    VERILOG,
    VHDL,
    VIML,
    XML,
    YACC,
    YAML,
    NR_BUILTIN_FILETYPES
} FileTypeEnum;

static const char *const builtin_filetype_names[NR_BUILTIN_FILETYPES] = {
    [NONE] = "none",
    [ADA] = "ada",
    [ASSEMBLY] = "asm",
    [AWK] = "awk",
    [BATCHFILE] = "batch",
    [BIBTEX] = "bibtex",
    [C] = "c",
    [CLOJURE] = "clojure",
    [CMAKE] = "cmake",
    [COFFEESCRIPT] = "coffeescript",
    [COMMONLISP] = "lisp",
    [CONFIG] = "config",
    [CPLUSPLUS] = "c",
    [CSHARP] = "csharp",
    [CSS] = "css",
    [CSV] = "csv",
    [DART] = "dart",
    [D] = "d",
    [DIFF] = "diff",
    [DOCKER] = "docker",
    [DOT] = "dot",
    [DOXYGEN] = "config",
    [DTERC] = "dte",
    [EMACSLISP] = "elisp",
    [EMAIL] = "mail",
    [GETTEXT] = "gettext",
    [GITCOMMIT] = "gitcommit",
    [GITREBASE] = "gitrebase",
    [GLSL] = "glsl",
    [GNUPLOT] = "gnuplot",
    [GO] = "go",
    [GPERF] = "gperf",
    [GROOVY] = "groovy",
    [HASKELL] = "haskell",
    [HTML] = "html",
    [INDENT] = "indent",
    [INI] = "ini",
    [INPUTRC] = "config",
    [JAVA] = "java",
    [JAVASCRIPT] = "javascript",
    [JSON] = "json",
    [LEX] = "lex",
    [LUA] = "lua",
    [M4] = "m4",
    [MAKE] = "make",
    [MALLARD] = "xml",
    [MARKDOWN] = "markdown",
    [MESON] = "meson",
    [MOONSCRIPT] = "moonscript",
    [NGINX] = "nginx",
    [NIM] = "nim",
    [NINJA] = "ninja",
    [NIX] = "nix",
    [OBJECTIVEC] = "objc",
    [PERL] = "perl",
    [PHP] = "php",
    [PKGCONFIG] = "pkg-config",
    [PROTOBUF] = "protobuf",
    [PYTHON] = "python",
    [RACKET] = "racket",
    [ROBOTSTXT] = "robotstxt",
    [ROFF] = "roff",
    [RUBY] = "ruby",
    [RUST] = "rust",
    [SCALA] = "scala",
    [SCHEME] = "scheme",
    [SCSS] = "scss",
    [SED] = "sed",
    [SHELL] = "sh",
    [SQL] = "sql",
    [TCL] = "tcl",
    [TEXINFO] = "texinfo",
    [TEXMFCNF] = "texmfcnf",
    [TEX] = "tex",
    [TOML] = "toml",
    [TYPESCRIPT] = "typescript",
    [VALA] = "vala",
    [VCARD] = "vcard",
    [VERILOG] = "verilog",
    [VHDL] = "vhdl",
    [VIML] = "viml",
    [XML] = "xml",
    [YACC] = "yacc",
    [YAML] = "yaml",
};

UNITTEST {
    for (size_t i = 0; i < ARRAY_COUNT(builtin_filetype_names); i++) {
        const char *const name = builtin_filetype_names[i];
        if (!name || name[0] == '\0') {
            BUG("missing value at builtin_filetype_names[%zu]", i);
        }
    }
}

#include "lookup/basenames.c"
#include "lookup/pathnames.c"
#include "lookup/extensions.c"
#include "lookup/interpreters.c"
#include "lookup/ignored-exts.c"

// Filetypes dynamically added via the `ft` command.
// Not grouped by name to make it possible to order them freely.
typedef struct {
    char *name;
    char *str;
    enum detect_type type;
} FileType;

static PointerArray filetypes = PTR_ARRAY_INIT;

void add_filetype(const char *name, const char *str, enum detect_type type)
{
    FileType *ft;
    regex_t re;

    switch (type) {
    case FT_CONTENT:
    case FT_FILENAME:
        if (!regexp_compile(&re, str, REG_NEWLINE | REG_NOSUB)) {
            return;
        }
        regfree(&re);
        break;
    default:
        break;
    }

    ft = xnew(FileType, 1);
    ft->name = xstrdup(name);
    ft->str = xstrdup(str);
    ft->type = type;
    ptr_array_add(&filetypes, ft);
}

// file.c.old~ -> c
// file..old   -> old
// file.old    -> old
static inline StringView get_ext(const char *const filename)
{
    StringView strview = STRING_VIEW_INIT;
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return strview;
    }

    ext++;
    size_t ext_len = strlen(ext);
    if (ext_len && ext[ext_len - 1] == '~') {
        ext_len--;
    }
    if (!ext_len) {
        return strview;
    }

    if (is_ignored_extension(ext, ext_len)) {
        int idx = -2;
        while (ext + idx >= filename) {
            if (ext[idx] == '.') {
                int len = -idx - 2;
                if (len) {
                    ext -= len + 1;
                    ext_len = len;
                }
                break;
            }
            idx--;
        }
    }

    strview.data = ext;
    strview.length = ext_len;
    return strview;
}

const char *find_ft (
    const char *filename,
    const char *interpreter,
    const char *first_line,
    size_t line_len
) {
    StringView path = STRING_VIEW_INIT;
    StringView ext = STRING_VIEW_INIT;
    StringView base = STRING_VIEW_INIT;
    StringView line = string_view(first_line, line_len);
    if (filename) {
        const char *b = path_basename(filename);
        ext = get_ext(b);
        base = string_view(b, strlen(b));
        path = string_view(filename, strlen(filename));
    }

    // Search user `ft` entries
    for (size_t i = 0; i < filetypes.count; i++) {
        const FileType *ft = filetypes.ptrs[i];
        switch (ft->type) {
        case FT_EXTENSION:
            if (!ext.length || !string_view_equal_cstr(&ext, ft->str)) {
                continue;
            }
            break;
        case FT_BASENAME:
            if (!base.length || !string_view_equal_cstr(&base, ft->str)) {
                continue;
            }
            break;
        case FT_FILENAME:
            if (
                !path.length
                || !regexp_match_nosub(ft->str, path.data, path.length)
            ) {
                continue;
            }
            break;
        case FT_CONTENT:
            if (
                !line.length
                || !regexp_match_nosub(ft->str, line.data, line.length)
            ) {
                continue;
            }
            break;
        case FT_INTERPRETER:
            if (!interpreter || !streq(interpreter, ft->str)) {
                continue;
            }
            break;
        }
        return ft->name;
    }

    // Search built-in hash tables
    if (interpreter) {
        size_t len = strlen(interpreter);
        FileTypeEnum ft = filetype_from_interpreter(interpreter, len);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (path.length) {
        FileTypeEnum ft = filetype_from_pathname(path.data, path.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (base.length) {
        FileTypeEnum ft = filetype_from_basename(base.data, base.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (line.length) {
        if (string_view_has_literal_prefix_icase(&line, "<!DOCTYPE HTML")) {
            return builtin_filetype_names[HTML];
        } else if (string_view_has_literal_prefix(&line, "[wrap-file]")) {
            return builtin_filetype_names[INI];
        } else if (string_view_has_literal_prefix(&line, "<?xml")) {
            return builtin_filetype_names[XML];
        }
    }

    if (ext.length) {
        FileTypeEnum ft = filetype_from_extension(ext.data, ext.length);
        if (ft) {
            return builtin_filetype_names[ft];
        }
    }

    if (string_view_has_literal_prefix(&path, "/etc/default/")) {
        return builtin_filetype_names[SHELL];
    } else if (string_view_has_literal_prefix(&path, "/etc/nginx/")) {
        return builtin_filetype_names[NGINX];
    }

    static const StringView conf = STRING_VIEW("conf");
    if (string_view_equal(&ext, &conf)) {
        if (string_view_has_literal_prefix(&path, "/etc/systemd/")) {
            return builtin_filetype_names[INI];
        } else if (string_view_has_literal_prefix(&path, "/etc/")) {
            return builtin_filetype_names[CONFIG];
        }
    }

    return NULL;
}

bool is_ft(const char *name)
{
    if (name[0] == '\0') {
        return false;
    }
    for (size_t i = 0; i < filetypes.count; i++) {
        const FileType *ft = filetypes.ptrs[i];
        if (streq(ft->name, name)) {
            return true;
        }
    }
    for (size_t i = 1; i < ARRAY_COUNT(builtin_filetype_names); i++) {
        if (streq(builtin_filetype_names[i], name)) {
            return true;
        }
    }
    return false;
}
