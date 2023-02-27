#include <stdlib.h>
#include "replace.h"
#include "buffer.h"
#include "change.h"
#include "editor.h"
#include "error.h"
#include "regexp.h"
#include "screen.h"
#include "selection.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/xmalloc.h"
#include "view.h"
#include "window.h"

static void build_replacement (
    String *buf,
    const char *line,
    const char *format,
    const regmatch_t *matches
) {
    for (size_t i = 0; format[i]; ) {
        char ch = format[i++];
        size_t match_idx;
        if (ch == '\\') {
            if (unlikely(format[i] == '\0')) {
                break;
            }
            ch = format[i++];
            if (ch < '1' || ch > '9') {
                string_append_byte(buf, ch);
                continue;
            }
            match_idx = ch - '0';
        } else if (ch == '&') {
            match_idx = 0;
        } else {
            string_append_byte(buf, ch);
            continue;
        }
        const regmatch_t *match = &matches[match_idx];
        regoff_t len = match->rm_eo - match->rm_so;
        if (len > 0) {
            string_append_buf(buf, line + match->rm_so, (size_t)len);
        }
    }
}

/*
 * s/abc/x
 *
 * string                to match against
 * -------------------------------------------
 * "foo abc bar abc baz" "foo abc bar abc baz"
 * "foo x bar abc baz"   " bar abc baz"
 */
static unsigned int replace_on_line (
    View *view,
    StringView *line,
    regex_t *re,
    const char *format,
    BlockIter *bi,
    ReplaceFlags *flagsp
) {
    const unsigned char *buf = line->data;
    unsigned char *alloc = NULL;
    EditorState *e = view->window->editor;
    ReplaceFlags flags = *flagsp;
    regmatch_t matches[32];
    size_t pos = 0;
    int eflags = 0;
    unsigned int nr = 0;

    while (regexp_exec (
        re,
        buf + pos,
        line->length - pos,
        ARRAYLEN(matches),
        matches,
        eflags
    )) {
        regoff_t match_len = matches[0].rm_eo - matches[0].rm_so;
        bool skip = false;

        // Move cursor to beginning of the text to replace
        block_iter_skip_bytes(bi, matches[0].rm_so);
        view->cursor = *bi;

        if (flags & REPLACE_CONFIRM) {
            switch (status_prompt(e, "Replace? [Y/n/a/q]", "ynaq")) {
            case 'y':
                break;
            case 'n':
                skip = true;
                break;
            case 'a':
                flags &= ~REPLACE_CONFIRM;
                *flagsp = flags;

                // Record rest of the changes as one chain
                begin_change_chain();
                break;
            case 'q':
            case 0:
                *flagsp = flags | REPLACE_CANCEL;
                goto out;
            }
        }

        if (skip) {
            // Move cursor after the matched text
            block_iter_skip_bytes(&view->cursor, match_len);
        } else {
            String b = STRING_INIT;
            build_replacement(&b, buf + pos, format, matches);

            // line ref is invalidated by modification
            if (buf == line->data && line->length != 0) {
                BUG_ON(alloc);
                alloc = xmemdup(buf, line->length);
                buf = alloc;
            }

            buffer_replace_bytes(view, match_len, b.buffer, b.len);
            nr++;

            // Update selection length
            if (view->selection) {
                view->sel_eo += b.len;
                view->sel_eo -= match_len;
            }

            // Move cursor after the replaced text
            block_iter_skip_bytes(&view->cursor, b.len);
            string_free(&b);
        }
        *bi = view->cursor;

        if (!match_len) {
            break;
        }

        if (!(flags & REPLACE_GLOBAL)) {
            break;
        }

        pos += matches[0].rm_so + match_len;

        // Don't match beginning of line again
        eflags = REG_NOTBOL;
    }

out:
    free(alloc);
    return nr;
}

bool reg_replace(View *view, const char *pattern, const char *format, ReplaceFlags flags)
{
    if (unlikely(pattern[0] == '\0')) {
        return error_msg("Search pattern must contain at least 1 character");
    }

    int re_flags = REG_NEWLINE;
    re_flags |= (flags & REPLACE_IGNORE_CASE) ? REG_ICASE : 0;
    re_flags |= (flags & REPLACE_BASIC) ? 0 : DEFAULT_REGEX_FLAGS;

    regex_t re;
    if (unlikely(!regexp_compile_internal(&re, pattern, re_flags))) {
        return false;
    }

    BlockIter bi = block_iter(view->buffer);
    size_t nr_bytes;
    bool swapped = false;
    if (view->selection) {
        SelectionInfo info;
        init_selection(view, &info);
        view->cursor = info.si;
        view->sel_so = info.so;
        view->sel_eo = info.eo;
        swapped = info.swapped;
        bi = view->cursor;
        nr_bytes = info.eo - info.so;
    } else {
        BlockIter eof = bi;
        block_iter_eof(&eof);
        nr_bytes = block_iter_get_offset(&eof);
    }

    // Record multiple changes as one chain only when replacing all
    if (!(flags & REPLACE_CONFIRM)) {
        begin_change_chain();
    }

    unsigned int nr_substitutions = 0;
    size_t nr_lines = 0;
    while (1) {
        StringView line;
        fill_line_ref(&bi, &line);

        // Number of bytes to process
        size_t count = line.length;
        if (line.length > nr_bytes) {
            // End of selection is not full line
            line.length = nr_bytes;
        }

        unsigned int nr = replace_on_line(view, &line, &re, format, &bi, &flags);
        if (nr) {
            nr_substitutions += nr;
            nr_lines++;
        }

        if (flags & REPLACE_CANCEL || count + 1 >= nr_bytes) {
            break;
        }

        nr_bytes -= count + 1;
        block_iter_next_line(&bi);
    }

    if (!(flags & REPLACE_CONFIRM)) {
        end_change_chain(view);
    }

    regfree(&re);

    if (nr_substitutions) {
        info_msg (
            "%u substitution%s on %zu line%s",
            nr_substitutions,
            (nr_substitutions > 1) ? "s" : "",
            nr_lines,
            (nr_lines > 1) ? "s" : ""
        );
    } else if (!(flags & REPLACE_CANCEL)) {
        error_msg("Pattern '%s' not found", pattern);
    }

    if (view->selection) {
        // Undo what init_selection() did
        if (view->sel_eo) {
            view->sel_eo--;
        }
        if (swapped) {
            ssize_t tmp = view->sel_so;
            view->sel_so = view->sel_eo;
            view->sel_eo = tmp;
        }
        block_iter_goto_offset(&view->cursor, view->sel_eo);
        view->sel_eo = SEL_EO_RECALC;
    }

    return (nr_substitutions > 0);
}
