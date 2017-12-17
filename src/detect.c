#include "detect.h"
#include "regexp.h"

static bool next_line(BlockIter *bi, LineRef *lr)
{
    if (!block_iter_eat_line(bi)) {
        return false;
    }
    fill_line_ref(bi, lr);
    return true;
}

/*
 * Parse #! line and return interpreter name without vesion number.
 * For example if file's first line is "#!/usr/bin/env python2" then
 * "python" is returned.
 */
char *detect_interpreter(Buffer *b)
{
    BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
    PointerArray m = PTR_ARRAY_INIT;
    LineRef lr;
    char *ret;

    fill_line_ref(&bi, &lr);
    if (!regexp_match (
        "^#! */.*(/env +|/)([a-zA-Z_-]+)[0-9.]*( |$)",
        lr.line,
        lr.size,
        &m
    )) {
        return NULL;
    }

    ret = xstrdup(m.ptrs[2]);
    ptr_array_free(&m);

    if (!streq(ret, "sh")) {
        return ret;
    }

    /*
     * #!/bin/sh
     * # the next line restarts using wish \
     * exec wish "$0" "$@"
     */
    if (
        !next_line(&bi, &lr)
        || !regexp_match_nosub("^#.*\\\\$", lr.line, lr.size)
    ) {
        return ret;
    }

    if (
        !next_line(&bi, &lr)
        || !regexp_match_nosub("^exec +wish +", lr.line, lr.size)
    ) {
        return ret;
    }

    free(ret);
    return xstrdup("wish");
}

static bool allow_odd_indent(const Buffer *b)
{
    // 1, 3, 5 and 7 space indent
    int odd = 1 << 0 | 1 << 2 | 1 << 4 | 1 << 6;
    return b->options.detect_indent & odd;
}

static int indent_len(const Buffer *b, const char *line, int len, bool *tab_indent)
{
    bool space_before_tab = false;
    int spaces = 0;
    int tabs = 0;
    int pos = 0;

    while (pos < len) {
        if (line[pos] == ' ') {
            spaces++;
        } else if (line[pos] == '\t') {
            tabs++;
            if (spaces) {
                space_before_tab = true;
            }
        } else {
            break;
        }
        pos++;
    }
    *tab_indent = false;
    if (pos == len) {
        // Whitespace only
        return -1;
    }
    if (pos == 0) {
        // Not indented
        return 0;
    }
    if (space_before_tab) {
        // Mixed indent
        return -2;
    }
    if (tabs) {
        // Tabs and possible spaces after tab for alignment
        *tab_indent = true;
        return tabs * 8;
    }
    if (len > spaces && line[spaces] == '*') {
        // '*' after indent, could be long C style comment
        if (spaces % 2 || allow_odd_indent(b)) {
            return spaces - 1;
        }
    }
    return spaces;
}

bool detect_indent(Buffer *b)
{
    BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
    int current_indent = 0;
    int counts[9] = {0};
    int tab_count = 0;
    int space_count = 0;

    for (unsigned int i = 0; i < 200; i++) {
        LineRef lr;
        int indent;
        bool tab;

        fill_line_ref(&bi, &lr);
        indent = indent_len(b, lr.line, lr.size, &tab);
        if (indent == -2) {
            // Ignore mixed indent because tab width might not be 8
        } else if (indent == -1) {
            // Empty line, no change in indent
        } else if (indent == 0) {
            current_indent = 0;
        } else {
            // Indented line
            int change;

            // Count only increase in indentation because indentation
            // almost always grows one level at time, whereas it can
            // can decrease multiple levels all at once.
            if (current_indent == -1) {
                current_indent = 0;
            }
            change = indent - current_indent;
            if (change > 0 && change <= 8) {
                counts[change]++;
            }

            if (tab) {
                tab_count++;
            } else {
                space_count++;
            }
            current_indent = indent;
        }

        if (!block_iter_next_line(&bi)) {
            break;
        }
    }
    if (tab_count == 0 && space_count == 0) {
        return false;
    }
    if (tab_count > space_count) {
        b->options.emulate_tab = false;
        b->options.expand_tab = false;
        b->options.indent_width = b->options.tab_width;
    } else {
        size_t m = 0;
        for (size_t i = 1; i < ARRAY_COUNT(counts); i++) {
            if (b->options.detect_indent & 1 << (i - 1)) {
                if (counts[i] > counts[m]) {
                    m = i;
                }
            }
        }
        if (m == 0) {
            return false;
        }
        b->options.emulate_tab = true;
        b->options.expand_tab = true;
        b->options.indent_width = m;
    }
    return true;
}
