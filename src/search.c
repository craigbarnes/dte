#include "search.h"
#include "buffer.h"
#include "change.h"
#include "error.h"
#include "regexp.h"
#include "selection.h"
#include "util/ascii.h"
#include "util/string.h"
#include "util/xmalloc.h"
#include "view.h"

static bool do_search_fwd(View *view, regex_t *regex, BlockIter *bi, bool skip)
{
    int flags = block_iter_is_bol(bi) ? 0 : REG_NOTBOL;

    do {
        if (block_iter_is_eof(bi)) {
            return false;
        }

        regmatch_t match;
        StringView line;
        fill_line_ref(bi, &line);

        // NOTE: If this is the first iteration then line.data contains
        // partial line (text starting from the cursor position) and
        // if match.rm_so is 0 then match is at beginning of the text
        // which is same as the cursor position.
        if (regexp_exec(regex, line.data, line.length, 1, &match, flags)) {
            if (skip && match.rm_so == 0) {
                // Ignore match at current cursor position
                regoff_t count = match.rm_eo;
                if (count == 0) {
                    // It is safe to skip one byte because every line
                    // has one extra byte (newline) that is not in line.data
                    count = 1;
                }
                block_iter_skip_bytes(bi, (size_t)count);
                return do_search_fwd(view, regex, bi, false);
            }

            block_iter_skip_bytes(bi, match.rm_so);
            view->cursor = *bi;
            view->center_on_scroll = true;
            view_reset_preferred_x(view);
            return true;
        }

        skip = false; // Not at cursor position any more
        flags = 0;
    } while (block_iter_next_line(bi));

    return false;
}

static bool do_search_bwd(View *view, regex_t *regex, BlockIter *bi, ssize_t cx, bool skip)
{
    if (block_iter_is_eof(bi)) {
        goto next;
    }

    do {
        regmatch_t match;
        StringView line;
        int flags = 0;
        regoff_t offset = -1;
        regoff_t pos = 0;

        fill_line_ref(bi, &line);
        while (
            pos <= line.length
            && regexp_exec(regex, line.data + pos, line.length - pos, 1, &match, flags)
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

bool search_tag(View *view, const char *pattern, bool *err)
{
    regex_t regex;
    bool found = false;
    if (!regexp_compile_basic(&regex, pattern, REG_NEWLINE)) {
        *err = true;
        return found;
    }

    BlockIter bi = BLOCK_ITER_INIT(&view->buffer->blocks);
    if (do_search_fwd(view, &regex, &bi, false)) {
        view->center_on_scroll = true;
        found = true;
    } else {
        // Don't center view to cursor unnecessarily
        view->force_center = false;
        error_msg("Tag not found");
        *err = true;
    }

    regfree(&regex);
    return found;
}

static void free_regex(SearchState *search)
{
    if (search->re_flags) {
        regfree(&search->regex);
        search->re_flags = 0;
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

static bool update_regex(SearchState *search, SearchCaseSensitivity cs)
{
    int re_flags = REG_NEWLINE;
    switch (cs) {
    case CSS_TRUE:
        break;
    case CSS_FALSE:
        re_flags |= REG_ICASE;
        break;
    case CSS_AUTO:
        if (!has_upper(search->pattern)) {
            re_flags |= REG_ICASE;
        }
        break;
    default:
        BUG("unhandled case sensitivity value");
    }

    if (re_flags == search->re_flags) {
        return true;
    }

    free_regex(search);

    search->re_flags = re_flags;
    if (regexp_compile(&search->regex, search->pattern, search->re_flags)) {
        return true;
    }

    free_regex(search);
    return false;
}

void search_set_regexp(SearchState *search, const char *pattern)
{
    free_regex(search);
    free(search->pattern);
    search->pattern = xstrdup(pattern);
}

static void do_search_next(EditorState *e, bool skip)
{
    SearchState *search = &e->search;
    if (!search->pattern) {
        error_msg("No previous search pattern");
        return;
    }
    if (!update_regex(search, e->options.case_sensitive_search)) {
        return;
    }

    View *view = e->view;
    BlockIter bi = view->cursor;
    regex_t *regex = &search->regex;
    if (search->direction == SEARCH_FWD) {
        if (do_search_fwd(view, regex, &bi, true)) {
            return;
        }

        block_iter_bof(&bi);
        if (do_search_fwd(view, regex, &bi, false)) {
            info_msg("Continuing at top");
        } else {
            error_msg("Pattern '%s' not found", search->pattern);
        }
    } else {
        size_t cursor_x = block_iter_bol(&bi);
        if (do_search_bwd(view, regex, &bi, cursor_x, skip)) {
            return;
        }

        block_iter_eof(&bi);
        if (do_search_bwd(view, regex, &bi, -1, false)) {
            info_msg("Continuing at bottom");
        } else {
            error_msg("Pattern '%s' not found", search->pattern);
        }
    }
}

void search_prev(EditorState *e)
{
    SearchDirection *dir = &e->search.direction;
    toggle_search_direction(dir);
    search_next(e);
    toggle_search_direction(dir);
}

void search_next(EditorState *e)
{
    do_search_next(e, false);
}

void search_next_word(EditorState *e)
{
    do_search_next(e, true);
}

static void build_replacement (
    String *buf,
    const char *line,
    const char *format,
    regmatch_t *matches
) {
    size_t i = 0;
    while (format[i]) {
        char ch = format[i++];
        if (ch == '\\') {
            if (format[i] >= '1' && format[i] <= '9') {
                int n = format[i++] - '0';
                int len = matches[n].rm_eo - matches[n].rm_so;
                if (len > 0) {
                    string_append_buf(buf, line + matches[n].rm_so, len);
                }
            } else if (format[i] != '\0') {
                string_append_byte(buf, format[i++]);
            }
        } else if (ch == '&') {
            int len = matches[0].rm_eo - matches[0].rm_so;
            if (len > 0) {
                string_append_buf(buf, line + matches[0].rm_so, len);
            }
        } else {
            string_append_byte(buf, ch);
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
    EditorState *e,
    StringView *line,
    regex_t *re,
    const char *format,
    BlockIter *bi,
    ReplaceFlags *flagsp
) {
    const unsigned char *buf = line->data;
    unsigned char *alloc = NULL;
    View *view = e->view;
    ReplaceFlags flags = *flagsp;
    regmatch_t matches[32];
    size_t pos = 0;
    int eflags = 0;
    unsigned int nr = 0;

    while (regexp_exec (
        re,
        buf + pos,
        line->length - pos,
        ARRAY_COUNT(matches),
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

void reg_replace(EditorState *e, const char *pattern, const char *format, ReplaceFlags flags)
{
    if (unlikely(pattern[0] == '\0')) {
        error_msg("Search pattern must contain at least 1 character");
        return;
    }

    int re_flags = REG_NEWLINE;
    re_flags |= (flags & REPLACE_IGNORE_CASE) ? REG_ICASE : 0;
    re_flags |= (flags & REPLACE_BASIC) ? 0 : REG_EXTENDED;

    regex_t re;
    if (unlikely(!regexp_compile_internal(&re, pattern, re_flags))) {
        return;
    }

    View *view = e->view;
    BlockIter bi = BLOCK_ITER_INIT(&view->buffer->blocks);
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

        unsigned int nr = replace_on_line(e, &line, &re, format, &bi, &flags);
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
        view->sel_eo = UINT_MAX;
    }
}
