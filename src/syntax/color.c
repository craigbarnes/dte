#include <stdlib.h>
#include <string.h>
#include "color.h"
#include "command/serialize.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/macros.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

const char builtin_color_names[NR_BC][16] = {
    [BC_ACTIVETAB] = "activetab",
    [BC_COMMANDLINE] = "commandline",
    [BC_CURRENTLINE] = "currentline",
    [BC_DEFAULT] = "default",
    [BC_DIALOG] = "dialog",
    [BC_ERRORMSG] = "errormsg",
    [BC_INACTIVETAB] = "inactivetab",
    [BC_INFOMSG] = "infomsg",
    [BC_LINENUMBER] = "linenumber",
    [BC_NOLINE] = "noline",
    [BC_NONTEXT] = "nontext",
    [BC_SELECTION] = "selection",
    [BC_STATUSLINE] = "statusline",
    [BC_TABBAR] = "tabbar",
    [BC_WSERROR] = "wserror",
};

UNITTEST {
    CHECK_BSEARCH_STR_ARRAY(builtin_color_names, strcmp);
}

static TermColor *find_real_color(ColorScheme *colors, const char *name)
{
    ssize_t idx = BSEARCH_IDX(name, builtin_color_names, vstrcmp);
    if (idx >= 0) {
        BUG_ON(idx >= ARRAYLEN(builtin_color_names));
        return &colors->builtin[idx];
    }
    return hashmap_get(&colors->other, name);
}

static const TermColor *find_real_color_const(const ColorScheme *colors, const char *name)
{
    return find_real_color((ColorScheme*)colors, name);
}

void set_highlight_color(ColorScheme *colors, const char *name, const TermColor *color)
{
    TermColor *c = find_real_color(colors, name);
    if (c) {
        *c = *color;
        return;
    }

    c = xnew(TermColor, 1);
    *c = *color;
    hashmap_insert(&colors->other, xstrdup(name), c);
}

const TermColor *find_color(const ColorScheme *colors, const char *name)
{
    const TermColor *c = find_real_color_const(colors, name);
    if (c) {
        return c;
    }

    const char *dot = strchr(name, '.');
    return dot ? find_real_color_const(colors, dot + 1) : NULL;
}

void clear_hl_colors(ColorScheme *colors)
{
    hashmap_clear(&colors->other, free);
}

typedef struct {
    const char *name;
    TermColor color;
} HlColor;

static int hlcolor_cmp(const void *ap, const void *bp)
{
    const HlColor *a = ap;
    const HlColor *b = bp;
    return strcmp(a->name, b->name);
}

void string_append_hl_color(String *s, const char *name, const TermColor *color)
{
    string_append_literal(s, "hi ");
    string_append_escaped_arg(s, name, true);
    string_append_byte(s, ' ');
    if (unlikely(name[0] == '-')) {
        string_append_literal(s, "-- ");
    }
    string_append_cstring(s, term_color_to_string(color));
}

String dump_hl_colors(const ColorScheme *colors)
{
    String buf = string_new(4096);
    string_append_literal(&buf, "# UI colors:\n");
    for (size_t i = 0; i < NR_BC; i++) {
        string_append_hl_color(&buf, builtin_color_names[i], &colors->builtin[i]);
        string_append_byte(&buf, '\n');
    }

    const HashMap *hl_colors = &colors->other;
    const size_t count = hl_colors->count;
    if (unlikely(count == 0)) {
        return buf;
    }

    // Copy the HashMap entries into an array
    HlColor *array = xnew(HlColor, count);
    size_t n = 0;
    for (HashMapIter it = hashmap_iter(hl_colors); hashmap_next(&it); ) {
        const TermColor *c = it.entry->value;
        array[n++] = (HlColor) {
            .name = it.entry->key,
            .color = *c,
        };
    }

    // Sort the array
    BUG_ON(n != count);
    qsort(array, count, sizeof(array[0]), hlcolor_cmp);

    string_append_literal(&buf, "\n# Syntax colors:\n");
    for (size_t i = 0; i < count; i++) {
        string_append_hl_color(&buf, array[i].name, &array[i].color);
        string_append_byte(&buf, '\n');
    }

    free(array);
    return buf;
}
