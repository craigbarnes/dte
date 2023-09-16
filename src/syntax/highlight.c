#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "highlight.h"
#include "syntax/merge.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static bool state_is_valid(const State *st)
{
    return ((uintptr_t)st & 1) == 0;
}

static void mark_state_invalid(void **ptrs, size_t idx)
{
    const State *st = ptrs[idx];
    ptrs[idx] = (State*)((uintptr_t)st | 1);
}

static bool states_equal(void **ptrs, size_t idx, const State *b)
{
    const State *a = (State*)((uintptr_t)ptrs[idx] & ~(uintptr_t)1);
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
    const StyleMap *sm,
    const char *delim,
    size_t len
) {
    delim = mem_intern(delim, len);
    for (size_t i = 0, n = state->heredoc.states.count; i < n; i++) {
        HeredocState *s = state->heredoc.states.ptrs[i];
        // Note: this tests string equality via (interned) pointer equality
        if (s->delim == delim) {
            BUG_ON(s->len != len);
            return s->state;
        }
    }

    SyntaxMerge m = {
        .subsyn = state->heredoc.subsyntax,
        .return_state = state->default_action.destination,
        .delim = delim,
        .delim_len = len
    };

    HeredocState *s = xnew(HeredocState, 1);
    *s = (HeredocState) {
        .state = merge_syntax(syn, &m, sm),
        .delim = delim,
        .len = len,
    };

    ptr_array_append(&state->heredoc.states, s);
    return s->state;
}

// Set styles in range [start,end] and return number of styles set
static size_t set_style_range (
    const TermStyle **styles,
    const Action *action,
    size_t start,
    size_t end
) {
    BUG_ON(start > end);
    const TermStyle *style = action->emit_style;
    for (size_t i = start; i < end; i++) {
        styles[i] = style;
    }
    return end - start;
}

// Line should be terminated with \n unless it's the last line
static const TermStyle **highlight_line (
    Syntax *syn,
    State *state,
    const StyleMap *sm,
    const StringView *line_sv,
    State **ret
) {
    static const TermStyle **styles;
    static size_t alloc;
    const unsigned char *const line = line_sv->data;
    const size_t len = line_sv->length;
    size_t i = 0;
    ssize_t sidx = -1;

    if (len > alloc) {
        alloc = round_size_to_next_multiple(len, 128);
        xrenew(styles, alloc);
    }

    top:
    if (i >= len) {
        BUG_ON(i > len);
        *ret = state;
        return styles;
    }

    for (size_t ci = 0, n = state->conds.count; ci < n; ci++) {
        const Condition *cond = state->conds.ptrs[ci];
        const ConditionData *u = &cond->u;
        const Action *a = &cond->a;
        switch (cond->type) {
        case COND_CHAR_BUFFER:
            if (!bitset_contains(u->bitset, line[i])) {
                break;
            }
            if (sidx < 0) {
                sidx = i;
            }
            styles[i++] = a->emit_style;
            state = a->destination;
            goto top;
        case COND_BUFIS:
            if (sidx < 0 || !bufis(u, line + sidx, i - sidx)) {
                break;
            }
            set_style_range(styles, a, sidx, i);
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_BUFIS_ICASE:
            if (sidx < 0 || !bufis_icase(u, line + sidx, i - sidx)) {
                break;
            }
            set_style_range(styles, a, sidx, i);
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_CHAR:
            if (!bitset_contains(u->bitset, line[i])) {
                break;
            }
            styles[i++] = a->emit_style;
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_CHAR1:
            if (u->ch != line[i]) {
                break;
            }
            styles[i++] = a->emit_style;
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_INLIST:
            if (sidx < 0 || !hashset_get(&u->str_list->strings, line + sidx, i - sidx)) {
                break;
            }
            set_style_range(styles, a, sidx, i);
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_INLIST_BUFFER:
            if (sidx < 0 || !hashset_get(&u->str_list->strings, line + sidx, i - sidx)) {
                break;
            }
            set_style_range(styles, a, sidx, i);
            state = a->destination;
            goto top;
        case COND_RECOLOR: {
            size_t start = (i >= u->recolor_len) ? i - u->recolor_len : 0;
            set_style_range(styles, a, start, i);
            } break;
        case COND_RECOLOR_BUFFER:
            if (sidx >= 0) {
                set_style_range(styles, a, sidx, i);
                sidx = -1;
            }
            break;
        case COND_STR: {
            size_t slen = u->str.len;
            size_t end = i + slen;
            if (len < end || !mem_equal(u->str.buf, line + i, slen)) {
                break;
            }
            i += set_style_range(styles, a, i, end);
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
            i += set_style_range(styles, a, i, end);
            sidx = -1;
            state = a->destination;
            goto top;
            }
        case COND_STR2:
            // Optimized COND_STR (length 2, case sensitive)
            if (len < i + 2 || !mem_equal(u->str.buf, line + i, 2)) {
                break;
            }
            styles[i++] = a->emit_style;
            styles[i++] = a->emit_style;
            sidx = -1;
            state = a->destination;
            goto top;
        case COND_HEREDOCEND: {
            const char *str = u->heredocend.data;
            size_t slen = u->heredocend.length;
            size_t end = i + slen;
            if (len >= end && (slen == 0 || mem_equal(str, line + i, slen))) {
                i += set_style_range(styles, a, i, end);
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
        styles[i++] = state->default_action.emit_style;
        // Fallthrough
    case STATE_NOEAT:
        sidx = -1;
        // Fallthrough
    case STATE_NOEAT_BUFFER:
        state = state->default_action.destination;
        break;
    case STATE_HEREDOCBEGIN:
        if (sidx < 0) {
            sidx = i;
        }
        state = handle_heredoc(syn, state, sm, line + sidx, i - sidx);
        break;
    case STATE_INVALID:
    default:
        BUG("unhandled default action type");
    }

    goto top;
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

static ssize_t fill_hole (
    Syntax *syn,
    PointerArray *line_start_states,
    const StyleMap *sm,
    BlockIter *bi,
    ssize_t sidx,
    ssize_t eidx
) {
    void **ptrs = line_start_states->ptrs;
    ssize_t idx = sidx;

    while (idx < eidx) {
        StringView line;
        State *st;
        fill_line_nl_ref(bi, &line);
        block_iter_eat_line(bi);
        highlight_line(syn, ptrs[idx++], sm, &line, &st);

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

void hl_fill_start_states (
    Syntax *syn,
    PointerArray *line_start_states,
    const StyleMap *sm,
    BlockIter *bi,
    size_t line_nr
) {
    if (!syn) {
        return;
    }

    PointerArray *s = line_start_states;
    ssize_t current_line = 0;
    ssize_t idx = 0;

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
        block_iter_move_down(bi, idx - current_line);
        current_line = idx;

        // NOTE: might not fill entire hole, which is ok
        ssize_t count = fill_hole(syn, s, sm, bi, idx, last);
        idx += count;
        current_line += count;
    }

    // Add new
    block_iter_move_down(bi, s->count - 1 - current_line);
    while (s->count - 1 < line_nr) {
        StringView line;
        fill_line_nl_ref(bi, &line);
        highlight_line (
            syn,
            states[s->count - 1],
            sm,
            &line,
            &states[s->count]
        );
        s->count++;
        block_iter_eat_line(bi);
    }
}

const TermStyle **hl_line (
    Syntax *syn,
    PointerArray *line_start_states,
    const StyleMap *sm,
    const StringView *line,
    size_t line_nr,
    bool *next_changed
) {
    *next_changed = false;
    if (!syn) {
        return NULL;
    }

    PointerArray *s = line_start_states;
    BUG_ON(line_nr >= s->count);
    State *next;
    const TermStyle **styles = highlight_line(syn, s->ptrs[line_nr++], sm, line, &next);

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
    return styles;
}

// Called after text has been inserted to re-highlight changed lines
void hl_insert(PointerArray *line_start_states, size_t first, size_t lines)
{
    PointerArray *s = line_start_states;
    if (first >= s->count) {
        // Nothing to re-highlight
        return;
    }

    size_t last = first + lines;
    if (last + 1 >= s->count) {
        // Last already highlighted lines changed; there's nothing to
        // gain, so throw them away
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
void hl_delete(PointerArray *line_start_states, size_t first, size_t deleted_nl)
{
    PointerArray *s = line_start_states;
    if (s->count == 1) {
        return;
    }

    if (first >= s->count) {
        // Nothing to highlight
        return;
    }

    size_t last = first + deleted_nl;
    if (last + 1 >= s->count) {
        // Last already highlighted lines changed; there's nothing to
        // gain, so throw them away
        s->count = first + 1;
        return;
    }

    // There are already highlighted lines after changed lines; try to
    // save the work

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
