#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "highlight.h"
#include "syntax.h"
#include "block-iter.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static bool state_is_valid(const State *st)
{
    return ((uintptr_t)st & 1) == 0;
}

static void mark_state_invalid(void **ptrs, size_t idx)
{
    const State *st = ptrs[idx];
    ptrs[idx] = (State *)((uintptr_t)st | 1);
}

static bool states_equal(void **ptrs, size_t idx, const State *b)
{
    const State *a = (State *)((uintptr_t)ptrs[idx] & ~(uintptr_t)1);
    return a == b;
}

static bool bufis(const ConditionData *u, const char *buf, size_t len)
{
    if (len != (size_t)u->str.len) {
        return false;
    }
    return mem_equal(u->str.buf, buf, len);
}

static bool bufis_icase(const ConditionData *u, const char *buf, size_t len)
{
    if (len != (size_t)u->str.len) {
        return false;
    }
    return mem_equal_icase(u->str.buf, buf, len);
}

static State *handle_heredoc (
    Syntax *syn,
    State *state,
    const char *delim,
    size_t len
) {
    for (size_t i = 0, n = state->heredoc.states.count; i < n; i++) {
        HeredocState *s = state->heredoc.states.ptrs[i];
        if (s->len == len && mem_equal(s->delim, delim, len)) {
            return s->state;
        }
    }

    SyntaxMerge m = {
        .subsyn = state->heredoc.subsyntax,
        .return_state = state->a.destination,
        .delim = delim,
        .delim_len = len
    };

    HeredocState *s = xnew(HeredocState, 1);
    *s = (HeredocState) {
        .state = merge_syntax(syn, &m),
        .delim = xmemdup(delim, len),
        .len = len,
    };

    ptr_array_append(&state->heredoc.states, s);
    return s->state;
}

// Line should be terminated with \n unless it's the last line
static TermColor **highlight_line (
    Syntax *syn,
    State *state,
    const StringView *line_sv,
    State **ret
) {
    static TermColor **colors;
    static size_t alloc;
    const unsigned char *const line = line_sv->data;
    const size_t len = line_sv->length;
    size_t i = 0;
    ssize_t sidx = -1;

    if (len > alloc) {
        alloc = round_size_to_next_multiple(len, 128);
        xrenew(colors, alloc);
    }

    while (1) {
        const Condition *cond;
        const ConditionData *u;
        const Action *a;
        unsigned char ch;
    top:
        if (i == len) {
            break;
        }
        ch = line[i];
        for (size_t ci = 0, n = state->conds.count; ci < n; ci++) {
            cond = state->conds.ptrs[ci];
            u = &cond->u;
            a = &cond->a;
            switch (cond->type) {
            case COND_CHAR_BUFFER:
                if (!bitset_contains(u->bitset, ch)) {
                    break;
                }
                if (sidx < 0) {
                    sidx = i;
                }
                colors[i++] = a->emit_color;
                state = a->destination;
                goto top;
            case COND_BUFIS:
                if (sidx >= 0 && bufis(u, line + sidx, i - sidx)) {
                    for (size_t idx = sidx; idx < i; idx++) {
                        colors[idx] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_BUFIS_ICASE:
                if (sidx >= 0 && bufis_icase(u, line + sidx, i - sidx)) {
                    for (size_t idx = sidx; idx < i; idx++) {
                        colors[idx] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_CHAR:
                if (!bitset_contains(u->bitset, ch)) {
                    break;
                }
                colors[i++] = a->emit_color;
                sidx = -1;
                state = a->destination;
                goto top;
            case COND_CHAR1:
                if (u->ch != ch) {
                    break;
                }
                colors[i++] = a->emit_color;
                sidx = -1;
                state = a->destination;
                goto top;
            case COND_INLIST:
                if (
                    sidx >= 0
                    && hashset_get(&u->str_list->strings, line + sidx, i - sidx)
                ) {
                    for (size_t idx = sidx; idx < i; idx++) {
                        colors[idx] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_RECOLOR: {
                ssize_t idx = i - u->recolor_len;
                if (idx < 0) {
                    idx = 0;
                }
                while (idx < i) {
                    colors[idx++] = a->emit_color;
                }
                } break;
            case COND_RECOLOR_BUFFER:
                if (sidx >= 0) {
                    while (sidx < i) {
                        colors[sidx++] = a->emit_color;
                    }
                    sidx = -1;
                }
                break;
            case COND_STR: {
                size_t slen = u->str.len;
                size_t end = i + slen;
                if (len < end || !mem_equal(u->str.buf, line + i, slen)) {
                    break;
                }
                while (i < end) {
                    colors[i++] = a->emit_color;
                }
                sidx = -1;
                state = a->destination;
                goto top;
                }
            case COND_STR_ICASE: {
                size_t slen = u->str.len;
                size_t end = i + slen;
                if (len < end || !mem_equal_icase(u->str.buf, line + i, slen)) {
                    break;
                }
                while (i < end) {
                    colors[i++] = a->emit_color;
                }
                sidx = -1;
                state = a->destination;
                goto top;
                }
            case COND_STR2:
                // Optimized COND_STR (length 2, case sensitive)
                if (
                    ch == u->str.buf[0]
                    && len - i > 1
                    && line[i + 1] == u->str.buf[1]
                ) {
                    colors[i++] = a->emit_color;
                    colors[i++] = a->emit_color;
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_HEREDOCEND: {
                const char *str = u->heredocend.data;
                size_t slen = u->heredocend.length;
                size_t end = i + slen;
                if (len >= end && (slen == 0 || mem_equal(str, line + i, slen))) {
                    while (i < end) {
                        colors[i++] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                } break;
            default:
                BUG("unhandled condition type");
            }
        }

        switch (state->type) {
        case STATE_EAT:
            colors[i++] = state->a.emit_color;
            // fallthrough
        case STATE_NOEAT:
            sidx = -1;
            // fallthrough
        case STATE_NOEAT_BUFFER:
            a = &state->a;
            state = a->destination;
            break;
        case STATE_HEREDOCBEGIN:
            if (sidx < 0) {
                sidx = i;
            }
            state = handle_heredoc(syn, state, line + sidx, i - sidx);
            break;
        case STATE_INVALID:
        default:
            BUG("unhandled default action type");
        }
    }

    if (ret) {
        *ret = state;
    }
    return colors;
}

static void resize_line_states(PointerArray *s, size_t count)
{
    if (s->alloc < count) {
        s->alloc = round_size_to_next_multiple(count, 64);
        xrenew(s->ptrs, s->alloc);
    }
}

static void move_line_states (
    PointerArray *s,
    size_t to,
    size_t from,
    size_t count
) {
    memmove(s->ptrs + to, s->ptrs + from, count * sizeof(*s->ptrs));
}

static void block_iter_move_down(BlockIter *bi, size_t count)
{
    while (count--) {
        block_iter_eat_line(bi);
    }
}

static ssize_t fill_hole(Buffer *b, BlockIter *bi, ssize_t sidx, ssize_t eidx)
{
    void **ptrs = b->line_start_states.ptrs;
    ssize_t idx = sidx;

    while (idx < eidx) {
        StringView line;
        State *st;
        fill_line_nl_ref(bi, &line);
        block_iter_eat_line(bi);
        highlight_line(b->syn, ptrs[idx++], &line, &st);

        if (ptrs[idx] == st) {
            // Was not invalidated and didn't change
            break;
        }

        if (states_equal(ptrs, idx, st)) {
            // Was invalidated and didn't change
            ptrs[idx] = st;
        } else {
            // Invalidated or not but changed anyway
            ptrs[idx] = st;
            if (idx == eidx) {
                mark_state_invalid(ptrs, idx + 1);
            }
        }
    }
    return idx - sidx;
}

void hl_fill_start_states(Buffer *b, size_t line_nr)
{
    BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
    PointerArray *s = &b->line_start_states;
    ssize_t current_line = 0;
    ssize_t idx = 0;

    if (!b->syn) {
        return;
    }

    // NOTE: "+ 2" so that you don't have to worry about overflow in fill_hole()
    resize_line_states(s, line_nr + 2);
    State **states = (State **)s->ptrs;

    // Update invalid
    ssize_t last = line_nr;
    if (last >= s->count) {
        last = s->count - 1;
    }
    while (1) {
        while (idx <= last && state_is_valid(states[idx])) {
            idx++;
        }
        if (idx > last) {
            break;
        }

        // Go to line before first hole
        idx--;
        block_iter_move_down(&bi, idx - current_line);
        current_line = idx;

        // NOTE: might not fill entire hole, which is ok
        ssize_t count = fill_hole(b, &bi, idx, last);
        idx += count;
        current_line += count;
    }

    // Add new
    block_iter_move_down(&bi, s->count - 1 - current_line);
    while (s->count - 1 < line_nr) {
        StringView line;
        fill_line_nl_ref(&bi, &line);
        highlight_line (
            b->syn,
            states[s->count - 1],
            &line,
            &states[s->count]
        );
        s->count++;
        block_iter_eat_line(&bi);
    }
}

TermColor **hl_line (
    Buffer *b,
    const StringView *line,
    size_t line_nr,
    bool *next_changed
) {
    *next_changed = false;
    if (!b->syn) {
        return NULL;
    }

    PointerArray *s = &b->line_start_states;
    BUG_ON(line_nr >= s->count);
    State *next;
    TermColor **colors = highlight_line(b->syn, s->ptrs[line_nr++], line, &next);

    if (line_nr == s->count) {
        resize_line_states(s, s->count + 1);
        s->ptrs[s->count++] = next;
        *next_changed = true;
    } else if (s->ptrs[line_nr] == next) {
        // Was not invalidated and didn't change
    } else if (states_equal(s->ptrs, line_nr, next)) {
        // Was invalidated and didn't change
        s->ptrs[line_nr] = next;
        // *next_changed = 1;
    } else {
        // Invalidated or not but changed anyway
        s->ptrs[line_nr] = next;
        *next_changed = true;
        if (line_nr + 1 < s->count) {
            mark_state_invalid(s->ptrs, line_nr + 1);
        }
    }
    return colors;
}

// Called after text has been inserted to re-highlight changed lines
void hl_insert(Buffer *b, size_t first, size_t lines)
{
    PointerArray *s = &b->line_start_states;
    size_t last = first + lines;

    if (first >= s->count) {
        // Nothing to re-highlight
        return;
    }

    if (last + 1 >= s->count) {
        // Last already highlighted lines changed.
        // There's nothing to gain, throw them away.
        s->count = first + 1;
        return;
    }

    // Add room for new line states
    if (lines) {
        size_t to = last + 1;
        size_t from = first + 1;
        resize_line_states(s, s->count + lines);
        move_line_states(s, to, from, s->count - from);
        s->count += lines;
    }

    // Invalidate start states of new and changed lines
    for (size_t i = first + 1; i <= last + 1; i++) {
        mark_state_invalid(s->ptrs, i);
    }
}

// Called after text has been deleted to re-highlight changed lines
void hl_delete(Buffer *b, size_t first, size_t deleted_nl)
{
    PointerArray *s = &b->line_start_states;
    size_t last = first + deleted_nl;

    if (s->count == 1) {
        return;
    }

    if (first >= s->count) {
        // Nothing to highlight
        return;
    }

    if (last + 1 >= s->count) {
        // Last already highlighted lines changed.
        // There's nothing to gain, throw them away.
        s->count = first + 1;
        return;
    }

    // There are already highlighted lines after changed lines.
    // Try to save the work.

    // Remove deleted lines (states)
    if (deleted_nl) {
        size_t to = first + 1;
        size_t from = last + 1;
        move_line_states(s, to, from, s->count - from);
        s->count -= deleted_nl;
    }

    // Invalidate line start state after the changed line
    mark_state_invalid(s->ptrs, first + 1);
}
