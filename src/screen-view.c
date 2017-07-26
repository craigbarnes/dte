#include "screen.h"
#include "view.h"
#include "uchar.h"
#include "obuf.h"
#include "selection.h"
#include "hl.h"

struct line_info {
	struct view *view;
	long line_nr;
	long offset;
	long sel_so;
	long sel_eo;

	const unsigned char *line;
	long size;
	long pos;
	long indent_size;
	long trailing_ws_offset;
	struct hl_color **colors;
};

static bool is_default_bg_color(int color)
{
	return color == builtin_colors[BC_DEFAULT]->bg || color < 0;
}

// like mask_color() but can change bg color only if it has not been changed yet
static void mask_color2(struct term_color *color, const struct term_color *over)
{
	if (over->fg != -2)
		color->fg = over->fg;
	if (over->bg != -2 && is_default_bg_color(color->bg))
		color->bg = over->bg;
	if (!(over->attr & ATTR_KEEP))
		color->attr = over->attr;
}

static void mask_selection_and_current_line(struct line_info *info, struct term_color *color)
{
	if (info->offset >= info->sel_so && info->offset < info->sel_eo) {
		mask_color(color, builtin_colors[BC_SELECTION]);
	} else if (info->line_nr == info->view->cy) {
		mask_color2(color, builtin_colors[BC_CURRENTLINE]);
	}
}

static bool is_non_text(unsigned int u)
{
	if (u < 0x20)
		return u != '\t' || options.display_special;
	if (u == 0x7f)
		return true;
	return u_is_unprintable(u);
}

static int get_ws_error_option(struct buffer *b)
{
	int flags = b->options.ws_error;

	if (flags & WSE_AUTO_INDENT) {
		if (b->options.expand_tab) {
			flags |= WSE_TAB_AFTER_INDENT | WSE_TAB_INDENT;
		} else {
			flags |= WSE_SPACE_INDENT;
		}
	}
	return flags;
}

static bool whitespace_error(struct line_info *info, unsigned int u, long i)
{
	struct view *v = info->view;
	int flags = get_ws_error_option(v->buffer);

	if (i >= info->trailing_ws_offset && flags & WSE_TRAILING) {
		// Trailing whitespace.
		if (info->line_nr != v->cy || v->cx < info->trailing_ws_offset)
			return true;
		// Cursor is on this line and on the whitespace or at eol. It would
		// be annoying if the line you are editing displays trailing
		// whitespace as an error.
	}

	if (u == '\t') {
		if (i < info->indent_size) {
			// in indentation
			if (flags & WSE_TAB_INDENT)
				return true;
		} else {
			if (flags & WSE_TAB_AFTER_INDENT)
				return true;
		}
	} else if (i < info->indent_size) {
		// space in indentation
		const char *line = info->line;
		int count = 0, pos = i;

		while (pos > 0 && line[pos - 1] == ' ')
			pos--;
		while (pos < info->size && line[pos] == ' ') {
			pos++;
			count++;
		}

		if (count >= v->buffer->options.tab_width) {
			// spaces used instead of tab
			if (flags & WSE_SPACE_INDENT)
				return true;
		} else if (pos < info->size && line[pos] == '\t') {
			// space before tab
			if (flags & WSE_SPACE_INDENT)
				return true;
		} else {
			// less than tab width spaces at end of indentation
			if (flags & WSE_SPACE_ALIGN)
				return true;
		}
	}
	return false;
}

static unsigned int screen_next_char(struct line_info *info)
{
	long count, pos = info->pos;
	unsigned int u = info->line[pos];
	struct term_color color;
	bool ws_error = false;

	if (likely(u < 0x80)) {
		info->pos++;
		count = 1;
		if (u == '\t' || u == ' ')
			ws_error = whitespace_error(info, u, pos);
	} else {
		u = u_get_nonascii(info->line, info->size, &info->pos);
		count = info->pos - pos;

		// highly annoying no-break space etc.?
		if (u_is_special_whitespace(u) && (info->view->buffer->options.ws_error & WSE_SPECIAL))
			ws_error = true;
	}

	if (info->colors && info->colors[pos]) {
		color = info->colors[pos]->color;
	} else {
		color = *builtin_colors[BC_DEFAULT];
	}
	if (is_non_text(u))
		mask_color(&color, builtin_colors[BC_NONTEXT]);
	if (ws_error)
		mask_color(&color, builtin_colors[BC_WSERROR]);
	mask_selection_and_current_line(info, &color);
	set_color(&color);

	info->offset += count;
	return u;
}

static void screen_skip_char(struct line_info *info)
{
	unsigned int u = info->line[info->pos++];

	info->offset++;
	if (likely(u < 0x80)) {
		if (likely(!u_is_ctrl(u))) {
			obuf.x++;
		} else if (u == '\t' && obuf.tab != TAB_CONTROL) {
			obuf.x += (obuf.x + obuf.tab_width) / obuf.tab_width * obuf.tab_width - obuf.x;
		} else {
			// control
			obuf.x += 2;
		}
	} else {
		long pos = info->pos;

		info->pos--;
		u = u_get_nonascii(info->line, info->size, &info->pos);
		obuf.x += u_char_width(u);
		info->offset += info->pos - pos;
	}
}

static bool is_notice(const char *word, int len)
{
	static const char * const words[] = { "fixme", "todo", "xxx" };
	int i;

	for (i = 0; i < ARRAY_COUNT(words); i++) {
		const char *w = words[i];
		if (strlen(w) == len && !strncasecmp(w, word, len))
			return true;
	}
	return false;
}

// highlight certain words inside comments
static void hl_words(struct line_info *info)
{
	struct hl_color *cc = find_color("comment");
	struct hl_color *nc = find_color("notice");
	int i, j, si, max;

	if (info->colors == NULL || cc == NULL || nc == NULL)
		return;

	i = info->pos;
	if (i >= info->size)
		return;

	// go to beginning of partially visible word inside comment
	while (i > 0 && info->colors[i] == cc && is_word_byte(info->line[i]))
		i--;

	// This should be more than enough. I'm too lazy to iterate characters
	// instead of bytes and calculate text width.
	max = info->pos + screen_w * 4 + 8;

	while (i < info->size) {
		if (info->colors[i] != cc || !is_word_byte(info->line[i])) {
			if (i > max)
				break;
			i++;
		} else {
			// beginning of a word inside comment
			si = i++;
			while (i < info->size && info->colors[i] == cc && is_word_byte(info->line[i]))
				i++;
			if (is_notice(info->line + si, i - si)) {
				for (j = si; j < i; j++)
					info->colors[j] = nc;
			}
		}
	}
}

static void line_info_init(struct line_info *info, struct view *v, struct block_iter *bi, long line_nr)
{
	memset(info, 0, sizeof(*info));
	info->view = v;
	info->line_nr = line_nr;
	info->offset = block_iter_get_offset(bi);

	if (!v->selection) {
		info->sel_so = -1;
		info->sel_eo = -1;
	} else if (v->sel_eo != UINT_MAX) {
		/* already calculated */
		info->sel_so = v->sel_so;
		info->sel_eo = v->sel_eo;
		BUG_ON(info->sel_so > info->sel_eo);
	} else {
		struct selection_info sel;

		init_selection(v, &sel);
		info->sel_so = sel.so;
		info->sel_eo = sel.eo;
	}
}

static void line_info_set_line(struct line_info *info, struct lineref *lr, struct hl_color **colors)
{
	int i;

	BUG_ON(lr->size == 0);
	BUG_ON(lr->line[lr->size - 1] != '\n');

	info->line = lr->line;
	info->size = lr->size - 1;
	info->pos = 0;
	info->colors = colors;

	for (i = 0; i < info->size; i++) {
		char ch = info->line[i];
		if (ch != '\t' && ch != ' ')
			break;
	}
	info->indent_size = i;

	info->trailing_ws_offset = INT_MAX;
	for (i = info->size - 1; i >= 0; i--) {
		char ch = info->line[i];
		if (ch != '\t' && ch != ' ')
			break;
		info->trailing_ws_offset = i;
	}
}

static void print_line(struct line_info *info)
{
	struct term_color color;
	unsigned int u;

	// Screen might be scrolled horizontally. Skip most invisible
	// characters using screen_skip_char() which is much faster than
	// buf_skip(screen_next_char(info)).
	//
	// There can be a wide character (tab, control code etc.) which is
	// partially visible and can't be skipped using screen_skip_char().
	while (obuf.x + 8 < obuf.scroll_x && info->pos < info->size)
		screen_skip_char(info);

	hl_words(info);

	while (info->pos < info->size) {
		BUG_ON(obuf.x > obuf.scroll_x + obuf.width);
		u = screen_next_char(info);
		if (!buf_put_char(u)) {
			// +1 for newline
			info->offset += info->size - info->pos + 1;
			return;
		}
	}

	if (options.display_special && obuf.x >= obuf.scroll_x) {
		// syntax highlighter highlights \n but use default color anyway
		color = *builtin_colors[BC_DEFAULT];
		mask_color(&color, builtin_colors[BC_NONTEXT]);
		mask_selection_and_current_line(info, &color);
		set_color(&color);
		buf_put_char('$');
	}

	color = *builtin_colors[BC_DEFAULT];
	mask_selection_and_current_line(info, &color);
	set_color(&color);
	info->offset++;
	buf_clear_eol();
}

void update_range(struct view *v, int y1, int y2)
{
	struct line_info info;
	struct block_iter bi = v->cursor;
	int i, got_line;

	buf_reset(v->window->edit_x, v->window->edit_w, v->vx);
	obuf.tab_width = v->buffer->options.tab_width;
	obuf.tab = options.display_special ? TAB_SPECIAL : TAB_NORMAL;

	for (i = 0; i < v->cy - y1; i++)
		block_iter_prev_line(&bi);
	for (i = 0; i < y1 - v->cy; i++)
		block_iter_eat_line(&bi);
	block_iter_bol(&bi);

	line_info_init(&info, v, &bi, y1);

	y1 -= v->vy;
	y2 -= v->vy;

	got_line = !block_iter_is_eof(&bi);
	hl_fill_start_states(v->buffer, info.line_nr);
	for (i = y1; got_line && i < y2; i++) {
		struct lineref lr;
		struct hl_color **colors;
		int next_changed;

		obuf.x = 0;
		buf_move_cursor(v->window->edit_x, v->window->edit_y + i);

		fill_line_nl_ref(&bi, &lr);
		colors = hl_line(v->buffer, lr.line, lr.size, info.line_nr, &next_changed);
		line_info_set_line(&info, &lr, colors);
		print_line(&info);

		got_line = block_iter_next_line(&bi);
		info.line_nr++;

		if (next_changed && i + 1 == y2 && y2 < v->window->edit_h) {
			// more lines need to be updated not because their
			// contents have changed but because their highlight
			// state has
			y2++;
		}
	}

	if (i < y2 && info.line_nr == v->cy) {
		// dummy empty line is shown only if cursor is on it
		struct term_color color = *builtin_colors[BC_DEFAULT];

		obuf.x = 0;
		mask_color2(&color, builtin_colors[BC_CURRENTLINE]);
		set_color(&color);

		buf_move_cursor(v->window->edit_x, v->window->edit_y + i++);
		buf_clear_eol();
	}

	if (i < y2)
		set_builtin_color(BC_NOLINE);
	for (; i < y2; i++) {
		obuf.x = 0;
		buf_move_cursor(v->window->edit_x, v->window->edit_y + i);
		buf_put_char('~');
		buf_clear_eol();
	}
}
