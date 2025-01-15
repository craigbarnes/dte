#include <stdlib.h>
#include "search.h"
#include "block-iter.h"
#include "buffer.h"
#include "editor.h"
#include "error.h"
#include "regexp.h"
#include "util/ascii.h"
#include "util/xmalloc.h"
#include "window.h"

// NOLINTNEXTLINE(misc-no-recursion)
static bool do_search_fwd(View *view, regex_t *regex, BlockIter *bi, bool skip)
{
    int flags = block_iter_is_bol(bi) ? 0 : REG_NOTBOL;

    do {
        if (block_iter_is_eof(bi)) {
            return false;
        }

        regmatch_t match;
        StringView line = block_iter_get_line(bi);

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
        int flags = 0;
        regoff_t offset = -1;
        regoff_t pos = 0;
        StringView line = block_iter_get_line(bi);

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

bool search_tag(View *view, const char *pattern)
{
    // DEFAULT_REGEX_FLAGS is not used here because pattern has been
    // escaped by parse_ex_pattern() for use as a POSIX BRE
    regex_t regex;
    int err = regcomp(&regex, pattern, REG_NEWLINE);
    if (unlikely(err)) {
        regexp_error_msg(&view->window->editor->err, &regex, pattern, err);
    }

    BlockIter bi = block_iter(view->buffer);
    bool found = do_search_fwd(view, &regex, &bi, false);
    regfree(&regex);

    if (!found) {
        // Don't center view to cursor unnecessarily
        view->force_center = false;
        return error_msg(&view->window->editor->err, "Tag not found");
    }

    view->center_on_scroll = true;
    return true;
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

static bool update_regex(SearchState *search, ErrorBuffer *ebuf, SearchCaseSensitivity cs)
{
    const char *pattern = search->pattern;
    bool icase = (cs == CSS_FALSE) || (cs == CSS_AUTO && !has_upper(pattern));
    int flags = REG_NEWLINE | (icase ? REG_ICASE : 0);
    if (flags == search->re_flags) {
        return true;
    }

    if (search->re_flags) {
        regfree(&search->regex);
        search->re_flags = 0;
    }

    if (regexp_compile(ebuf, &search->regex, pattern, flags)) {
        search->re_flags = flags;
        return true;
    }

    regfree(&search->regex);
    return false;
}

void search_free_regexp(SearchState *search)
{
    if (search->re_flags) {
        regfree(&search->regex);
        search->re_flags = 0;
    }
    free(search->pattern);
}

void search_set_regexp(SearchState *search, const char *pattern)
{
    search_free_regexp(search);
    search->pattern = xstrdup(pattern);
}

bool do_search_next(View *view, SearchState *search, SearchCaseSensitivity cs, bool skip)
{
    ErrorBuffer *ebuf = &view->window->editor->err;
    if (!search->pattern) {
        return error_msg(ebuf, "No previous search pattern");
    }
    if (!update_regex(search, ebuf, cs)) {
        return false;
    }

    BlockIter bi = view->cursor;
    regex_t *regex = &search->regex;
    if (!search->reverse) {
        if (do_search_fwd(view, regex, &bi, true)) {
            return true;
        }
        block_iter_bof(&bi);
        if (do_search_fwd(view, regex, &bi, false)) {
            return info_msg(ebuf, "Continuing at top");
        }
    } else {
        size_t cursor_x = block_iter_bol(&bi);
        if (do_search_bwd(view, regex, &bi, cursor_x, skip)) {
            return true;
        }
        block_iter_eof(&bi);
        if (do_search_bwd(view, regex, &bi, -1, false)) {
            return info_msg(ebuf, "Continuing at bottom");
        }
    }

    return error_msg(ebuf, "Pattern '%s' not found", search->pattern);
}
