#include <stdlib.h>
#include "search.h"
#include "block-iter.h"
#include "buffer.h"
#include "regexp.h"
#include "util/ascii.h"
#include "util/xmalloc.h"

// Recurses at most once
// NOLINTNEXTLINE(misc-no-recursion)
static bool do_search_fwd(BlockIter *bi, const regex_t *regex, bool skip)
{
    int flags = block_iter_is_bol(bi) ? 0 : REG_NOTBOL;

    do {
        if (block_iter_is_eof(bi)) {
            return false;
        }

        regmatch_t match;
        StringView line = block_iter_get_line(bi);

        // NOTE: If this is the first iteration then `line.data` contains
        // a partial line (text starting from the cursor position) and if
        // `match.rm_so` is 0 then the match is at the beginning of the
        // text, which is the same as the cursor position.
        if (regexp_exec(regex, line, 1, &match, flags)) {
            if (skip && match.rm_so == 0) {
                // Ignore match at current cursor position

                // It's always safe to skip one byte, because every line
                // has a newline that's not included in line.data
                block_iter_skip_bytes(bi, MAX(match.rm_eo, 1));

                return do_search_fwd(bi, regex, false);
            }

            block_iter_skip_bytes(bi, match.rm_so);
            return true;
        }

        skip = false; // Not at cursor position any more
        flags = 0;
    } while (block_iter_next_line(bi));

    return false;
}

static bool do_search_bwd(BlockIter *bi, const regex_t *regex, ssize_t cx, bool skip)
{
    if (block_iter_is_eof(bi)) {
        goto next;
    }

    do {
        const StringView line = block_iter_get_line(bi);
        StringView slice = line;
        regmatch_t match;
        int flags = 0;
        regoff_t offset = -1;
        regoff_t pos = 0;

        while (pos <= line.length && regexp_exec(regex, slice, 1, &match, flags)) {
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
            slice = strview_suffix(line, pos);

            if (match.rm_so == match.rm_eo) {
                // Zero length match
                break;
            }
        }

        if (offset >= 0) {
            block_iter_skip_bytes(bi, offset);
            return true;
        }

        next:
        cx = -1;
    } while (block_iter_prev_line(bi));

    return false;
}

static bool search_fwd(View *view, BlockIter *bi, const regex_t *regex, bool skip)
{
    if (!do_search_fwd(bi, regex, skip)) {
        return false;
    }

    view->cursor = *bi;
    view->center_on_scroll = true;
    view_reset_preferred_x(view);
    return true;
}

static bool search_bwd(View *view, BlockIter *bi, const regex_t *regex, ssize_t cx, bool skip)
{
    if (!do_search_bwd(bi, regex, cx, skip)) {
        return false;
    }

    view->cursor = *bi;
    view->center_on_scroll = true;
    view_reset_preferred_x(view);
    return true;
}

bool search_tag(View *view, ErrorBuffer *ebuf, const char *pattern)
{
    // DEFAULT_REGEX_FLAGS is not used here because pattern has been
    // escaped by parse_ex_pattern() for use as a POSIX BRE
    regex_t regex;
    int err = regcomp(&regex, pattern, REG_NEWLINE);
    if (unlikely(err)) {
        regexp_error_msg(ebuf, &regex, pattern, err);
    }

    BlockIter bi = block_iter(view->buffer);
    bool found = search_fwd(view, &bi, &regex, false);
    regfree(&regex);

    if (!found) {
        // Don't center view to cursor unnecessarily
        view->force_center = false;
        return error_msg(ebuf, "Tag not found");
    }

    view->center_on_scroll = true;
    return true;
}

static bool has_upper(const char *str)
{
    return strview_contains_char_type(strview(str), ASCII_UPPER);
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

bool do_search_next(View *view, SearchState *search, ErrorBuffer *ebuf, SearchCaseSensitivity cs, bool skip)
{
    if (!search->pattern) {
        return error_msg(ebuf, "No previous search pattern");
    }
    if (!update_regex(search, ebuf, cs)) {
        return false;
    }

    BlockIter bi = view->cursor;
    regex_t *regex = &search->regex;
    if (!search->reverse) {
        if (search_fwd(view, &bi, regex, true)) {
            return true;
        }
        block_iter_bof(&bi);
        if (search_fwd(view, &bi, regex, false)) {
            return info_msg(ebuf, "Continuing at top");
        }
    } else {
        size_t cursor_x = block_iter_bol(&bi);
        if (search_bwd(view, &bi, regex, cursor_x, skip)) {
            return true;
        }
        block_iter_eof(&bi);
        if (search_bwd(view, &bi, regex, -1, false)) {
            return info_msg(ebuf, "Continuing at bottom");
        }
    }

    return error_msg(ebuf, "Pattern '%s' not found", search->pattern);
}
