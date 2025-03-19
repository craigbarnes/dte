#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "command/serialize.h"
#include "util/array.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/xmalloc.h"

static const char builtin_style_names[NR_BSE][16] = {
    [BSE_ACTIVETAB] = "activetab",
    [BSE_COMMANDLINE] = "commandline",
    [BSE_CURRENTLINE] = "currentline",
    [BSE_DEFAULT] = "default",
    [BSE_DIALOG] = "dialog",
    [BSE_ERRORMSG] = "errormsg",
    [BSE_INACTIVETAB] = "inactivetab",
    [BSE_INFOMSG] = "infomsg",
    [BSE_LINENUMBER] = "linenumber",
    [BSE_NOLINE] = "noline",
    [BSE_NONTEXT] = "nontext",
    [BSE_SELECTION] = "selection",
    [BSE_STATUSLINE] = "statusline",
    [BSE_TABBAR] = "tabbar",
    [BSE_WSERROR] = "wserror",
};

UNITTEST {
    CHECK_BSEARCH_STR_ARRAY(builtin_style_names, strcmp);
}

static TermStyle *find_real_style(StyleMap *styles, const char *name)
{
    ssize_t idx = BSEARCH_IDX(name, builtin_style_names, vstrcmp);
    if (idx >= 0) {
        BUG_ON(idx >= ARRAYLEN(builtin_style_names));
        return &styles->builtin[idx];
    }
    return hashmap_get(&styles->other, name);
}

static const TermStyle *find_real_style_const(const StyleMap *styles, const char *name)
{
    return find_real_style((StyleMap*)styles, name);
}

void set_highlight_style(StyleMap *styles, const char *name, const TermStyle *style)
{
    TermStyle *existing = find_real_style(styles, name);
    if (existing) {
        *existing = *style;
    } else {
        hashmap_insert(&styles->other, xstrdup(name), XMEMDUP(style));
    }
}

const TermStyle *find_style(const StyleMap *styles, const char *name)
{
    const TermStyle *style = find_real_style_const(styles, name);
    if (style) {
        return style;
    }

    const char *dot = strchr(name, '.');
    return dot ? find_real_style_const(styles, dot + 1) : NULL;
}

void clear_hl_styles(StyleMap *styles)
{
    hashmap_clear(&styles->other, free);
}

void collect_builtin_styles(PointerArray *a, const char *prefix)
{
    COLLECT_STRINGS(builtin_style_names, a, prefix);
}

typedef struct {
    const char *name;
    TermStyle style;
} HlStyle;

static int hlstyle_cmp(const void *ap, const void *bp)
{
    const HlStyle *a = ap;
    const HlStyle *b = bp;
    return strcmp(a->name, b->name);
}

void string_append_hl_style(String *s, const char *name, const TermStyle *style)
{
    string_append_literal(s, "hi ");
    string_append_escaped_arg(s, name, true);
    string_append_byte(s, ' ');
    if (unlikely(name[0] == '-')) {
        string_append_literal(s, "-- ");
    }
    char buf[TERM_STYLE_BUFSIZE];
    string_append_cstring(s, term_style_to_string(buf, style));
}

String dump_hl_styles(const StyleMap *styles)
{
    String buf = string_new(4096);
    string_append_literal(&buf, "# UI colors:\n");
    for (size_t i = 0; i < NR_BSE; i++) {
        string_append_hl_style(&buf, builtin_style_names[i], &styles->builtin[i]);
        string_append_byte(&buf, '\n');
    }

    const HashMap *hl_styles = &styles->other;
    const size_t count = hl_styles->count;
    if (unlikely(count == 0)) {
        return buf;
    }

    // Copy the HashMap entries into an array
    HlStyle *array = xmallocarray(count, sizeof(*array));
    size_t n = 0;
    for (HashMapIter it = hashmap_iter(hl_styles); hashmap_next(&it); ) {
        const TermStyle *style = it.entry->value;
        array[n++] = (HlStyle) {
            .name = it.entry->key,
            .style = *style,
        };
    }

    // Sort the array
    BUG_ON(n != count);
    qsort(array, count, sizeof(array[0]), hlstyle_cmp);

    string_append_literal(&buf, "\n# Syntax colors:\n");
    for (size_t i = 0; i < count; i++) {
        string_append_hl_style(&buf, array[i].name, &array[i].style);
        string_append_byte(&buf, '\n');
    }

    free(array);
    return buf;
}
