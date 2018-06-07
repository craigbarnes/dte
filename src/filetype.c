#include "filetype.h"
#include "common.h"
#include "macros.h"
#include "regexp.h"
#include "path.h"
#include "ptr-array.h"
#include "string-view.h"

typedef enum {
    NONE = 0,
    ADA,
    ASSEMBLY,
    AWK,
    BATCHFILE,
    C,
    CLOJURE,
    CMAKE,
    COFFEESCRIPT,
    COMMONLISP,
    CPLUSPLUS,
    CSHARP,
    CSS,
    D,
    DART,
    DIFF,
    DOCKER,
    DOXYGEN,
    DTERC,
    EMACSLISP,
    EMAIL,
    GITCOMMIT,
    GITREBASE,
    GLSL,
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
    MAKE,
    MALLARD,
    MARKDOWN,
    MESON,
    MOONSCRIPT,
    NGINX,
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
    SCHEME,
    SCSS,
    SED,
    SHELL,
    SQL,
    TCL,
    TEX,
    TEXMFCNF,
    TOML,
    VALA,
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
    [C] = "c",
    [CLOJURE] = "clojure",
    [CMAKE] = "cmake",
    [COFFEESCRIPT] = "cmake",
    [COMMONLISP] = "lisp",
    [CPLUSPLUS] = "c",
    [CSHARP] = "csharp",
    [CSS] = "css",
    [DART] = "dart",
    [D] = "d",
    [DIFF] = "diff",
    [DOCKER] = "docker",
    [DOXYGEN] = "config",
    [DTERC] = "dte",
    [EMACSLISP] = "elisp",
    [EMAIL] = "mail",
    [GITCOMMIT] = "gitcommit",
    [GITREBASE] = "gitrebase",
    [GLSL] = "glsl",
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
    [MAKE] = "make",
    [MALLARD] = "xml",
    [MARKDOWN] = "markdown",
    [MESON] = "meson",
    [MOONSCRIPT] = "moon",
    [NGINX] = "nginx",
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
    [SCHEME] = "scheme",
    [SCSS] = "scss",
    [SED] = "sed",
    [SHELL] = "sh",
    [SQL] = "sql",
    [TCL] = "tcl",
    [TEXMFCNF] = "texmfcnf",
    [TEX] = "tex",
    [TOML] = "toml",
    [VALA] = "vala",
    [VIML] = "viml",
    [XML] = "xml",
    [YACC] = "yacc",
    [YAML] = "yaml",
};

#if (DEBUG >= 1) && defined(CONSTRUCTOR)
static void CONSTRUCTOR sanity_check(void)
{
    for (size_t i = 0; i < ARRAY_COUNT(builtin_filetype_names); i++) {
        const char *const name = builtin_filetype_names[i];
        if (!name || name[0] == '\0') {
            BUG("missing value at builtin_filetype_names[%zu]", i);
        }
    }
}
#endif

typedef struct {
    const char *const key;
    const FileTypeEnum filetype;
} FileTypeHashSlot;

#include "lookup/basenames.c"
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
    size_t filename_len = 0;
    const char *base = NULL;
    StringView ext = STRING_VIEW_INIT;
    if (filename) {
        base = path_basename(filename);
        ext = get_ext(base);
        filename_len = strlen(filename);
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
            if (!base || !streq(base, ft->str)) {
                continue;
            }
            break;
        case FT_FILENAME:
            if (
                !filename
                || !regexp_match_nosub(ft->str, filename, filename_len)
            ) {
                continue;
            }
            break;
        case FT_CONTENT:
            if (
                !first_line
                || !regexp_match_nosub(ft->str, first_line, line_len)
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
    const FileTypeHashSlot *slot;
    if (interpreter) {
        slot = filetype_from_interpreter(interpreter, strlen(interpreter));
        if (slot) {
            return builtin_filetype_names[slot->filetype];
        }
    }
    if (base) {
        slot = filetype_from_basename(base, strlen(base));
        if (slot) {
            return builtin_filetype_names[slot->filetype];
        }
    }
    if (ext.length) {
        slot = filetype_from_extension(ext.data, ext.length);
        if (slot) {
            return builtin_filetype_names[slot->filetype];
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
