#include "highlight.h"
#include "syntax.h"

#include <inttypes.h>

static bool state_is_valid(const State *st)
{
    return ((uintptr_t)st & 1) == 0;
}

static void mark_state_invalid(void **ptrs, int idx)
{
    State *st = ptrs[idx];
    ptrs[idx] = (State *)((uintptr_t)st | 1);
}

static bool states_equal(void **ptrs, int idx, const State *b)
{
    State *a = (State *)((uintptr_t)ptrs[idx] & ~(uintptr_t)1);
    return a == b;
}

static int bitmap_get(const unsigned char *bitmap, unsigned int idx)
{
    unsigned int byte = idx / 8;
    unsigned int bit = idx & 7;
    return bitmap[byte] & 1 << bit;
}

static bool is_buffered(const Condition *cond, const char *str, size_t len)
{
    if (len != cond->u.cond_bufis.len) {
        return false;
    }

    if (cond->u.cond_bufis.icase) {
        return !strncasecmp(cond->u.cond_bufis.str, str, len);
    }
    return !memcmp(cond->u.cond_bufis.str, str, len);
}

static bool in_hash(StringList *list, const char *str, size_t len)
{
    unsigned long hash = buf_hash(str, len);
    HashStr *h = list->hash[hash % ARRAY_COUNT(list->hash)];

    if (list->icase) {
        while (h) {
            if (len == h->len && !strncasecmp(str, h->str, len)) {
                return true;
            }
            h = h->next;
        }
    } else {
        while (h) {
            if (len == h->len && !memcmp(str, h->str, len)) {
                return true;
            }
            h = h->next;
        }
    }
    return false;
}

static State *handle_heredoc (
    Syntax *syn,
    State *state,
    const char *delim,
    size_t len
) {
    HeredocState *s;
    SyntaxMerge m;

    for (size_t i = 0; i < state->heredoc.states.count; i++) {
        s = state->heredoc.states.ptrs[i];
        if (s->len == len && !memcmp(s->delim, delim, len)) {
            return s->state;
        }
    }

    m.subsyn = state->heredoc.subsyntax;
    m.return_state = state->a.destination;
    m.delim = delim;
    m.delim_len = len;

    s = xnew0(HeredocState, 1);
    s->state = merge_syntax(syn, &m);
    s->delim = xmemdup(delim, len);
    s->len = len;
    ptr_array_add(&state->heredoc.states, s);
    return s->state;
}

// Line should be terminated with \n unless it's the last line
static HlColor **highlight_line (
    Syntax *syn,
    State *state,
    const char *line,
    size_t len,
    State **ret
) {
    static HlColor **colors;
    static size_t alloc;
    size_t i = 0;
    int sidx = -1;

    if (len > alloc) {
        alloc = ROUND_UP(len, 128);
        xrenew(colors, alloc);
    }

    while (1) {
        const Condition *cond;
        const Action *a;
        unsigned char ch;
    top:
        if (i == len) {
            break;
        }
        ch = line[i];
        for (size_t ci = 0; ci < state->conds.count; ci++) {
            cond = state->conds.ptrs[ci];
            a = &cond->a;
            switch (cond->type) {
            case COND_CHAR_BUFFER:
                if (!bitmap_get(cond->u.cond_char.bitmap, ch)) {
                    break;
                }
                if (sidx < 0) {
                    sidx = i;
                }
                colors[i++] = a->emit_color;
                state = a->destination;
                goto top;
            case COND_BUFIS:
                if (sidx >= 0 && is_buffered(cond, line + sidx, i - sidx)) {
                    int idx;
                    for (idx = sidx; idx < i; idx++) {
                        colors[idx] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_CHAR:
                if (!bitmap_get(cond->u.cond_char.bitmap, ch)) {
                    break;
                }
                colors[i++] = a->emit_color;
                sidx = -1;
                state = a->destination;
                goto top;
            case COND_INLIST:
                if (
                    sidx >= 0
                    && in_hash(cond->u.cond_inlist.list, line + sidx, i - sidx)
                ) {
                    int idx;
                    for (idx = sidx; idx < i; idx++) {
                        colors[idx] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_RECOLOR: {
                int idx = i - cond->u.cond_recolor.len;
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
                size_t slen = cond->u.cond_str.len;
                size_t end = i + slen;
                if (
                    len >= end
                    && !memcmp(cond->u.cond_str.str, line + i, slen)
                ) {
                    while (i < end) {
                        colors[i++] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                } break;
            case COND_STR_ICASE: {
                size_t slen = cond->u.cond_str.len;
                size_t end = i + slen;
                if (
                    len >= end
                    && !strncasecmp(cond->u.cond_str.str, line + i, slen)
                ) {
                    while (i < end) {
                        colors[i++] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                } break;
            case COND_STR2:
                // Optimized COND_STR (length 2, case sensitive)
                if (
                    ch == cond->u.cond_str.str[0]
                    && len - i > 1
                    && line[i + 1] == cond->u.cond_str.str[1]
                ) {
                    colors[i++] = a->emit_color;
                    colors[i++] = a->emit_color;
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                break;
            case COND_HEREDOCEND: {
                const char *str = cond->u.cond_heredocend.str;
                size_t slen = cond->u.cond_heredocend.len;
                size_t end = i + slen;
                if (len >= end && (slen == 0 || !memcmp(str, line + i, slen))) {
                    while (i < end) {
                        colors[i++] = a->emit_color;
                    }
                    sidx = -1;
                    state = a->destination;
                    goto top;
                }
                } break;
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
            BUG("Invalid default action type should never make it here");
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
        s->alloc = ROUND_UP(count, 64);
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

static int fill_hole(Buffer *b, BlockIter *bi, int sidx, int eidx)
{
    void **ptrs = b->line_start_states.ptrs;
    int idx = sidx;

    while (idx < eidx) {
        LineRef lr;
        State *st;

        fill_line_nl_ref(bi, &lr);
        block_iter_eat_line(bi);
        highlight_line(b->syn, ptrs[idx++], lr.line, lr.size, &st);

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

void hl_fill_start_states(Buffer *b, int line_nr)
{
    BlockIter bi = BLOCK_ITER_INIT(&b->blocks);
    PointerArray *s = &b->line_start_states;
    State **states;
    int current_line = 0;
    int idx = 0;
    int last;

    if (b->syn == NULL) {
        return;
    }

    // NOTE: "+ 2" so that you don't have to worry about overflow in fill_hole()
    resize_line_states(s, line_nr + 2);
    states = (State **)s->ptrs;

    // Update invalid
    last = line_nr;
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

        // NOTE: might not fill entire hole which is ok
        int count = fill_hole(b, &bi, idx, last);
        idx += count;
        current_line += count;
    }

    // Add new
    block_iter_move_down(&bi, s->count - 1 - current_line);
    while (s->count - 1 < line_nr) {
        LineRef lr;

        fill_line_nl_ref(&bi, &lr);
        highlight_line (
            b->syn,
            states[s->count - 1],
            lr.line,
            lr.size,
            &states[s->count]
        );
        s->count++;
        block_iter_eat_line(&bi);
    }
}

HlColor **hl_line (
    Buffer *b,
    const char *line,
    size_t len,
    int line_nr,
    int *next_changed
) {
    PointerArray *s = &b->line_start_states;
    HlColor **colors;
    State *next;

    *next_changed = 0;
    if (b->syn == NULL) {
        return NULL;
    }

    BUG_ON(line_nr >= s->count);
    colors = highlight_line(b->syn, s->ptrs[line_nr++], line, len, &next);

    if (line_nr == s->count) {
        resize_line_states(s, s->count + 1);
        s->ptrs[s->count++] = next;
        *next_changed = 1;
    } else if (s->ptrs[line_nr] == next) {
        // Was not invalidated and didn't change
    } else if (states_equal(s->ptrs, line_nr, next)) {
        // Was invalidated and didn't change
        s->ptrs[line_nr] = next;
        // *next_changed = 1;
    } else {
        // Invalidated or not but changed anyway
        s->ptrs[line_nr] = next;
        *next_changed = 1;
        if (line_nr + 1 < s->count) {
            mark_state_invalid(s->ptrs, line_nr + 1);
        }
    }
    return colors;
}

// Called after text has been inserted to re-highlight changed lines
void hl_insert(Buffer *b, int first, int lines)
{
    PointerArray *s = &b->line_start_states;
    int i, last = first + lines;

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
        int to = last + 1;
        int from = first + 1;
        resize_line_states(s, s->count + lines);
        move_line_states(s, to, from, s->count - from);
        s->count += lines;
    }

    // Invalidate start states of new and changed lines
    for (i = first + 1; i <= last + 1; i++) {
        mark_state_invalid(s->ptrs, i);
    }
}

// Called after text has been deleted to re-highlight changed lines
void hl_delete(Buffer *b, int first, int deleted_nl)
{
    PointerArray *s = &b->line_start_states;
    int last = first + deleted_nl;

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
        int to = first + 1;
        int from = last + 1;
        move_line_states(s, to, from, s->count - from);
        s->count -= deleted_nl;
    }

    // Invalidate line start state after the changed line
    mark_state_invalid(s->ptrs, first + 1);
}
