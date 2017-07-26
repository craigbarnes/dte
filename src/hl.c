#include "hl.h"
#include "buffer.h"
#include "syntax.h"

#include <inttypes.h>

static bool state_is_valid(const struct state *st)
{
	return ((uintptr_t)st & 1) == 0;
}

static void mark_state_invalid(void **ptrs, int idx)
{
	struct state *st = ptrs[idx];
	ptrs[idx] = (struct state *)((uintptr_t)st | 1);
}

static bool states_equal(void **ptrs, int idx, const struct state *b)
{
	struct state *a = (struct state *)((uintptr_t)ptrs[idx] & ~(uintptr_t)1);
	return a == b;
}

static int bitmap_get(const unsigned char *bitmap, unsigned int idx)
{
	unsigned int byte = idx / 8;
	unsigned int bit = idx & 7;
	return bitmap[byte] & 1 << bit;
}

static bool is_buffered(const struct condition *cond, const char *str, int len)
{
	if (len != cond->u.cond_bufis.len)
		return false;

	if (cond->u.cond_bufis.icase)
		return !strncasecmp(cond->u.cond_bufis.str, str, len);
	return !memcmp(cond->u.cond_bufis.str, str, len);
}

static bool in_hash(struct string_list *list, const char *str, int len)
{
	unsigned int hash = buf_hash(str, len);
	struct hash_str *h = list->hash[hash % ARRAY_COUNT(list->hash)];

	if (list->icase) {
		while (h) {
			if (len == h->len && !strncasecmp(str, h->str, len))
				return true;
			h = h->next;
		}
	} else {
		while (h) {
			if (len == h->len && !memcmp(str, h->str, len))
				return true;
			h = h->next;
		}
	}
	return false;
}

static struct state *handle_heredoc(struct syntax *syn, struct state *state, const char *delim, int len)
{
	struct heredoc_state *s;
	struct syntax_merge m;
	int i;

	for (i = 0; i < state->heredoc.states.count; i++) {
		s = state->heredoc.states.ptrs[i];
		if (s->len == len && !memcmp(s->delim, delim, len))
			return s->state;
	}

	m.subsyn = state->heredoc.subsyntax;
	m.return_state = state->a.destination;
	m.delim = delim;
	m.delim_len = len;

	s = xnew0(struct heredoc_state, 1);
	s->state = merge_syntax(syn, &m);
	s->delim = xmemdup(delim, len);
	s->len = len;
	ptr_array_add(&state->heredoc.states, s);
	return s->state;
}

// line should be terminated with \n unless it's the last line
static struct hl_color **highlight_line(struct syntax *syn, struct state *state, const char *line, int len, struct state **ret)
{
	static struct hl_color **colors;
	static int alloc;
	int i = 0, sidx = -1;

	if (len > alloc) {
		alloc = ROUND_UP(len, 128);
		xrenew(colors, alloc);
	}

	while (1) {
		const struct condition *cond;
		const struct action *a;
		unsigned char ch;
		int ci;
	top:
		if (i == len)
			break;
		ch = line[i];
		for (ci = 0; ci < state->conds.count; ci++) {
			cond = state->conds.ptrs[ci];
			a = &cond->a;
			switch (cond->type) {
			case COND_CHAR_BUFFER:
				if (!bitmap_get(cond->u.cond_char.bitmap, ch))
					break;
				if (sidx < 0)
					sidx = i;
				colors[i++] = a->emit_color;
				state = a->destination;
				goto top;
			case COND_BUFIS:
				if (sidx >= 0 && is_buffered(cond, line + sidx, i - sidx)) {
					int idx;
					for (idx = sidx; idx < i; idx++)
						colors[idx] = a->emit_color;
					sidx = -1;
					state = a->destination;
					goto top;
				}
				break;
			case COND_CHAR:
				if (!bitmap_get(cond->u.cond_char.bitmap, ch))
					break;
				colors[i++] = a->emit_color;
				sidx = -1;
				state = a->destination;
				goto top;
			case COND_INLIST:
				if (sidx >= 0 && in_hash(cond->u.cond_inlist.list, line + sidx, i - sidx)) {
					int idx;
					for (idx = sidx; idx < i; idx++)
						colors[idx] = a->emit_color;
					sidx = -1;
					state = a->destination;
					goto top;
				}
				break;
			case COND_RECOLOR: {
				int idx = i - cond->u.cond_recolor.len;
				if (idx < 0)
					idx = 0;
				while (idx < i)
					colors[idx++] = a->emit_color;
				} break;
			case COND_RECOLOR_BUFFER:
				if (sidx >= 0) {
					while (sidx < i)
						colors[sidx++] = a->emit_color;
					sidx = -1;
				}
				break;
			case COND_STR: {
				int slen = cond->u.cond_str.len;
				int end = i + slen;
				if (len >= end && !memcmp(cond->u.cond_str.str, line + i, slen)) {
					while (i < end)
						colors[i++] = a->emit_color;
					sidx = -1;
					state = a->destination;
					goto top;
				}
				} break;
			case COND_STR_ICASE: {
				int slen = cond->u.cond_str.len;
				int end = i + slen;
				if (len >= end && !strncasecmp(cond->u.cond_str.str, line + i, slen)) {
					while (i < end)
						colors[i++] = a->emit_color;
					sidx = -1;
					state = a->destination;
					goto top;
				}
				} break;
			case COND_STR2:
				// optimized COND_STR (length 2, case sensitive)
				if (ch == cond->u.cond_str.str[0] && len - i > 1 &&
						line[i + 1] == cond->u.cond_str.str[1]) {
					colors[i++] = a->emit_color;
					colors[i++] = a->emit_color;
					sidx = -1;
					state = a->destination;
					goto top;
				}
				break;
			case COND_HEREDOCEND: {
				int slen = cond->u.cond_heredocend.len;
				int end = i + slen;
				if (len >= end && !memcmp(cond->u.cond_heredocend.str, line + i, slen)) {
					while (i < end)
						colors[i++] = a->emit_color;
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
			if (sidx < 0)
				sidx = i;
			state = handle_heredoc(syn, state, line + sidx, i - sidx);
			break;
		}
	}

	if (ret)
		*ret = state;
	return colors;
}

static void resize_line_states(struct ptr_array *s, unsigned int count)
{
	if (s->alloc < count) {
		s->alloc = ROUND_UP(count, 64);
		xrenew(s->ptrs, s->alloc);
	}
}

static void move_line_states(struct ptr_array *s, int to, int from, int count)
{
	memmove(s->ptrs + to, s->ptrs + from, count * sizeof(*s->ptrs));
}

static void block_iter_move_down(struct block_iter *bi, int count)
{
	while (count--)
		block_iter_eat_line(bi);
}

static int fill_hole(struct buffer *b, struct block_iter *bi, int sidx, int eidx)
{
	void **ptrs = b->line_start_states.ptrs;
	int idx = sidx;

	while (idx < eidx) {
		struct lineref lr;
		struct state *st;

		fill_line_nl_ref(bi, &lr);
		block_iter_eat_line(bi);
		highlight_line(b->syn, ptrs[idx++], lr.line, lr.size, &st);

		if (ptrs[idx] == st) {
			// was not invalidated and didn't change
			break;
		}

		if (states_equal(ptrs, idx, st)) {
			// was invalidated and didn't change
			ptrs[idx] = st;
		} else {
			// invalidated or not but changed anyway
			ptrs[idx] = st;
			if (idx == eidx)
				mark_state_invalid(ptrs, idx + 1);
		}
	}
	return idx - sidx;
}

void hl_fill_start_states(struct buffer *b, int line_nr)
{
	BLOCK_ITER(bi, &b->blocks);
	struct ptr_array *s = &b->line_start_states;
	struct state **states;
	int current_line = 0;
	int idx = 0;
	int last;

	if (b->syn == NULL)
		return;

	// NOTE: "+ 2" so that you don't have to worry about overflow in fill_hole()
	resize_line_states(s, line_nr + 2);
	states = (struct state **)s->ptrs;

	// update invalid
	last = line_nr;
	if (last >= s->count)
		last = s->count - 1;
	while (1) {
		int count;

		while (idx <= last && state_is_valid(states[idx]))
			idx++;
		if (idx > last)
			break;

		// go to line before first hole
		idx--;
		block_iter_move_down(&bi, idx - current_line);
		current_line = idx;

		// NOTE: might not fill entire hole which is ok
		count = fill_hole(b, &bi, idx, last);
		idx += count;
		current_line += count;
	}

	// add new
	block_iter_move_down(&bi, s->count - 1 - current_line);
	while (s->count - 1 < line_nr) {
		struct lineref lr;

		fill_line_nl_ref(&bi, &lr);
		highlight_line(b->syn, states[s->count - 1], lr.line, lr.size, &states[s->count]);
		s->count++;
		block_iter_eat_line(&bi);
	}
}

struct hl_color **hl_line(struct buffer *b, const char *line, int len, int line_nr, int *next_changed)
{
	struct ptr_array *s = &b->line_start_states;
	struct hl_color **colors;
	struct state *next;

	*next_changed = 0;
	if (b->syn == NULL)
		return NULL;

	BUG_ON(line_nr >= s->count);
	colors = highlight_line(b->syn, s->ptrs[line_nr++], line, len, &next);

	if (line_nr == s->count) {
		resize_line_states(s, s->count + 1);
		s->ptrs[s->count++] = next;
		*next_changed = 1;
	} else if (s->ptrs[line_nr] == next) {
		// was not invalidated and didn't change
	} else if (states_equal(s->ptrs, line_nr, next)) {
		// was invalidated and didn't change
		s->ptrs[line_nr] = next;
		//*next_changed = 1;
	} else {
		// invalidated or not but changed anyway
		s->ptrs[line_nr] = next;
		*next_changed = 1;
		if (line_nr + 1 < s->count)
			mark_state_invalid(s->ptrs, line_nr + 1);
	}
	return colors;
}

// called after text have been inserted to rehighlight changed lines
void hl_insert(struct buffer *b, int first, int lines)
{
	struct ptr_array *s = &b->line_start_states;
	int i, last = first + lines;

	if (first >= s->count) {
		// nothing to rehighlight
		return;
	}

	if (last + 1 >= s->count) {
		// last already highlighted lines changed
		// there's nothing to gain, throw them away
		s->count = first + 1;
		return;
	}

	// add room for new line states
	if (lines) {
		int to = last + 1;
		int from = first + 1;
		resize_line_states(s, s->count + lines);
		move_line_states(s, to, from, s->count - from);
		s->count += lines;
	}

	// invalidate start states of new and changed lines
	for (i = first + 1; i <= last + 1; i++)
		mark_state_invalid(s->ptrs, i);
}

// called after text have been deleted to rehighlight changed lines
void hl_delete(struct buffer *b, int first, int deleted_nl)
{
	struct ptr_array *s = &b->line_start_states;
	int last = first + deleted_nl;

	if (s->count == 1)
		return;

	if (first >= s->count) {
		// nothing to highlight
		return;
	}

	if (last + 1 >= s->count) {
		// last already highlighted lines changed
		// there's nothing to gain, throw them away
		s->count = first + 1;
		return;
	}

	// there are already highlighted lines after changed lines
	// try to save the work.

	// remove deleted lines (states)
	if (deleted_nl) {
		int to = first + 1;
		int from = last + 1;
		move_line_states(s, to, from, s->count - from);
		s->count -= deleted_nl;
	}

	// invalidate line start state after the changed line
	mark_state_invalid(s->ptrs, first + 1);
}
