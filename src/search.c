#include "search.h"
#include "buffer.h"
#include "view.h"
#include "editor.h"
#include "change.h"
#include "error.h"
#include "edit.h"
#include "str.h"
#include "regexp.h"
#include "selection.h"

#define MAX_SUBSTRINGS 32

static bool do_search_fwd(regex_t *regex, BlockIter *bi, bool skip)
{
    int flags = block_iter_is_bol(bi) ? 0 : REG_NOTBOL;

    do {
        regmatch_t match;
        LineRef lr;

        if (block_iter_is_eof(bi)) {
            return false;
        }

        fill_line_ref(bi, &lr);

        // NOTE: If this is the first iteration then lr.line contains
        // partial line (text starting from the cursor position) and
        // if match.rm_so is 0 then match is at beginning of the text
        // which is same as the cursor position.
        if (regexp_exec(regex, lr.line, lr.size, 1, &match, flags)) {
            if (skip && match.rm_so == 0) {
                // Ignore match at current cursor position
                long count = match.rm_eo;
                if (count == 0) {
                    // It is safe to skip one byte because every line
                    // has one extra byte (newline) that is not in lr.line
                    count = 1;
                }
                block_iter_skip_bytes(bi, count);
                return do_search_fwd(regex, bi, false);
            }

            block_iter_skip_bytes(bi, match.rm_so);
            view->cursor = *bi;
            view->center_on_scroll = true;
            view_reset_preferred_x(view);
            return true;
        }
        skip = false; // Not at cursor position anymore
        flags = 0;
    } while (block_iter_next_line(bi));
    return false;
}

static bool do_search_bwd(regex_t *regex, BlockIter *bi, int cx, bool skip)
{
    if (block_iter_is_eof(bi)) {
        goto next;
    }

    do {
        regmatch_t match;
        LineRef lr;
        int flags = 0;
        long offset = -1;
        long pos = 0;

        fill_line_ref(bi, &lr);
        while (
            pos <= lr.size
            && regexp_exec(regex, lr.line + pos, lr.size - pos, 1, &match, flags)
        ) {
            flags = REG_NOTBOL;
            if (cx >= 0) {
                if (pos + match.rm_so >= cx) {
                    // Ignore match at or after cursor
                    break;
                }
                if (skip && pos + match.rm_eo > cx) {
                    // Search -rw should not find word under cursor
                    break;
                }
            }

            // This might be what we want (last match before cursor)
            offset = pos + match.rm_so;
            pos += match.rm_eo;

            if (match.rm_so == match.rm_eo) {
                // Zero length match
                break;
            }
        }

        if (offset >= 0) {
            block_iter_skip_bytes(bi, offset);
            view->cursor = *bi;
            view->center_on_scroll = true;
            view_reset_preferred_x(view);
            return true;
        }
next:
        cx = -1;
    } while (block_iter_prev_line(bi));
    return false;
}

bool search_tag(const char *pattern, bool *err)
{
    BlockIter bi = BLOCK_ITER_INIT(&buffer->blocks);
    regex_t regex;
    bool found = false;

    if (!regexp_compile_basic(&regex, pattern, REG_NEWLINE)) {
        *err = true;
    } else if (do_search_fwd(&regex, &bi, false)) {
        view->center_on_scroll = true;
        found = true;
    } else {
        // Don't center view to cursor unnecessarily
        view->force_center = false;
        error_msg("Tag not found.");
        *err = true;
    }
    regfree(&regex);
    return found;
}

static struct {
    regex_t regex;
    char *pattern;
    SearchDirection direction;

    // If zero then regex hasn't been compiled
    int re_flags;
} current_search;

void search_set_direction(SearchDirection dir)
{
    current_search.direction = dir;
}

SearchDirection current_search_direction(void)
{
    return current_search.direction;
}

static void free_regex(void)
{
    if (current_search.re_flags) {
        regfree(&current_search.regex);
        current_search.re_flags = 0;
    }
}

static bool has_upper(const char *str)
{
    for (size_t i = 0; str[i]; i++) {
        if (ascii_isupper(str[i])) {
            return true;
        }
    }
    return false;
}

static bool update_regex(void)
{
    int re_flags = REG_NEWLINE;

    switch (editor.options.case_sensitive_search) {
    case CSS_TRUE:
        break;
    case CSS_FALSE:
        re_flags |= REG_ICASE;
        break;
    case CSS_AUTO:
        if (!has_upper(current_search.pattern)) {
            re_flags |= REG_ICASE;
        }
        break;
    }

    if (re_flags == current_search.re_flags) {
        return true;
    }

    free_regex();

    current_search.re_flags = re_flags;
    if (regexp_compile (
        &current_search.regex,
        current_search.pattern,
        current_search.re_flags
    )) {
        return true;
    }

    free_regex();
    return false;
}

void search_set_regexp(const char *pattern)
{
    free_regex();
    free(current_search.pattern);
    current_search.pattern = xstrdup(pattern);
}

static void do_search_next(bool skip)
{
    BlockIter bi = view->cursor;

    if (!current_search.pattern) {
        error_msg("No previous search pattern.");
        return;
    }
    if (!update_regex()) {
        return;
    }
    if (current_search.direction == SEARCH_FWD) {
        if (do_search_fwd(&current_search.regex, &bi, true)) {
            return;
        }

        block_iter_bof(&bi);
        if (do_search_fwd(&current_search.regex, &bi, false)) {
            info_msg("Continuing at top.");
        } else {
            info_msg("Pattern '%s' not found.", current_search.pattern);
        }
    } else {
        int cursor_x = block_iter_bol(&bi);

        if (do_search_bwd(&current_search.regex, &bi, cursor_x, skip)) {
            return;
        }

        block_iter_eof(&bi);
        if (do_search_bwd(&current_search.regex, &bi, -1, false)) {
            info_msg("Continuing at bottom.");
        } else {
            info_msg("Pattern '%s' not found.", current_search.pattern);
        }
    }
}

void search_prev(void)
{
    current_search.direction ^= 1;
    search_next();
    current_search.direction ^= 1;
}

void search_next(void)
{
    do_search_next(false);
}

void search_next_word(void)
{
    do_search_next(true);
}

static void build_replacement (
    String *buf,
    const char *line,
    const char *format,
    regmatch_t *m
) {
    size_t i = 0;

    while (format[i]) {
        int ch = format[i++];

        if (ch == '\\') {
            if (format[i] >= '1' && format[i] <= '9') {
                int n = format[i++] - '0';
                int len = m[n].rm_eo - m[n].rm_so;
                if (len > 0) {
                    string_add_buf(buf, line + m[n].rm_so, len);
                }
            } else {
                string_add_byte(buf, format[i++]);
            }
        } else if (ch == '&') {
            int len = m[0].rm_eo - m[0].rm_so;
            if (len > 0) {
                string_add_buf(buf, line + m[0].rm_so, len);
            }
        } else {
            string_add_byte(buf, ch);
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
static int replace_on_line (
    LineRef *lr,
    regex_t *re,
    const char *format,
    BlockIter *bi,
    ReplaceFlags *flagsp
) {
    unsigned char *buf = (unsigned char *)lr->line;
    ReplaceFlags flags = *flagsp;
    regmatch_t m[MAX_SUBSTRINGS];
    size_t pos = 0;
    int eflags = 0;
    int nr = 0;

    while (regexp_exec (
        re,
        buf + pos,
        lr->size - pos,
        MAX_SUBSTRINGS,
        m,
        eflags
    )) {
        int match_len = m[0].rm_eo - m[0].rm_so;
        bool skip = false;

        // Move cursor to beginning of the text to replace
        block_iter_skip_bytes(bi, m[0].rm_so);
        view->cursor = *bi;

        if (flags & REPLACE_CONFIRM) {
            switch (get_confirmation("Ynaq", "Replace?")) {
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

            build_replacement(&b, buf + pos, format, m);

            // lineref is invalidated by modification
            if (buf == lr->line) {
                buf = xmemdup(buf, lr->size);
            }

            buffer_replace_bytes(match_len, b.buffer, b.len);
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

        pos += m[0].rm_so + match_len;

        // Don't match beginning of line again
        eflags = REG_NOTBOL;
    }
out:
    if (buf != lr->line) {
        free(buf);
    }
    return nr;
}

void reg_replace(const char *pattern, const char *format, ReplaceFlags flags)
{
    BlockIter bi = BLOCK_ITER_INIT(&buffer->blocks);
    unsigned int nr_bytes;
    bool swapped = false;
    int re_flags = REG_NEWLINE;
    int nr_substitutions = 0;
    int nr_lines = 0;
    regex_t re;

    if (flags & REPLACE_IGNORE_CASE) {
        re_flags |= REG_ICASE;
    }
    if (flags & REPLACE_BASIC) {
        if (!regexp_compile_basic(&re, pattern, re_flags)) {
            return;
        }
    } else {
        if (!regexp_compile(&re, pattern, re_flags)) {
            return;
        }
    }

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

    while (1) {
        // Number of bytes to process
        long count;
        LineRef lr;
        int nr;

        fill_line_ref(&bi, &lr);
        count = lr.size;
        if (lr.size > nr_bytes) {
            // End of selection is not full line
            lr.size = nr_bytes;
        }

        nr = replace_on_line(&lr, &re, format, &bi, &flags);
        if (nr) {
            nr_substitutions += nr;
            nr_lines++;
        }
        if (flags & REPLACE_CANCEL) {
            break;
        }
        if (count + 1 >= nr_bytes) {
            break;
        }
        nr_bytes -= count + 1;

        BUG_ON(!block_iter_next_line(&bi));
    }

    if (!(flags & REPLACE_CONFIRM)) {
        end_change_chain();
    }

    regfree(&re);

    if (nr_substitutions) {
        info_msg("%d substitutions on %d lines.", nr_substitutions, nr_lines);
    } else if (!(flags & REPLACE_CANCEL)) {
        info_msg("Pattern '%s' not found.", pattern);
    }

    if (view->selection) {
        // Undo what init_selection() did
        if (view->sel_eo) {
            view->sel_eo--;
        }
        if (swapped) {
            long tmp = view->sel_so;
            view->sel_so = view->sel_eo;
            view->sel_eo = tmp;
        }
        block_iter_goto_offset(&view->cursor, view->sel_eo);
        view->sel_eo = UINT_MAX;
    }
}
