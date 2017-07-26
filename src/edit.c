#include "edit.h"
#include "move.h"
#include "buffer.h"
#include "view.h"
#include "change.h"
#include "gbuf.h"
#include "indent.h"
#include "uchar.h"
#include "regexp.h"
#include "selection.h"

struct paragraph_formatter {
	struct gbuf buf;
	char *indent;
	int indent_len;
	int indent_width;
	int cur_width;
	int text_width;
};

static char *copy_buf;
static long copy_len;
static bool copy_is_lines;

static const char *spattern = "\\{\\s*(//.*|/\\*.*\\*/\\s*)?$";
static const char *epattern = "^\\s*\\}";

/*
 * Stupid { ... } block selector.
 *
 * Because braces can be inside strings or comments and writing real
 * parser for many programming languages does not make sense the rules
 * for selecting a block are made very simple. Line that matches \{\s*$
 * starts a block and line that matches ^\s*\} ends it.
 */
void select_block(void)
{
	struct block_iter sbi, ebi, bi = view->cursor;
	struct lineref lr;
	int level = 0;

	// If current line does not match \{\s*$ but matches ^\s*\} then
	// cursor is likely at end of the block you want to select.
	fetch_this_line(&bi, &lr);
	if (!regexp_match_nosub(spattern, lr.line, lr.size) &&
	     regexp_match_nosub(epattern, lr.line, lr.size))
		block_iter_prev_line(&bi);

	while (1) {
		fetch_this_line(&bi, &lr);
		if (regexp_match_nosub(spattern, lr.line, lr.size)) {
			if (level++ == 0) {
				sbi = bi;
				block_iter_next_line(&bi);
				break;
			}
		}
		if (regexp_match_nosub(epattern, lr.line, lr.size))
			level--;

		if (!block_iter_prev_line(&bi))
			return;
	}

	while (1) {
		fetch_this_line(&bi, &lr);
		if (regexp_match_nosub(epattern, lr.line, lr.size)) {
			if (--level == 0) {
				ebi = bi;
				break;
			}
		}
		if (regexp_match_nosub(spattern, lr.line, lr.size))
			level++;

		if (!block_iter_next_line(&bi))
			return;
	}

	view->cursor = sbi;
	view->sel_so = block_iter_get_offset(&ebi);
	view->sel_eo = UINT_MAX;
	view->selection = SELECT_LINES;

	mark_all_lines_changed(buffer);
}

static int get_indent_of_matching_brace(void)
{
	struct block_iter bi = view->cursor;
	struct lineref lr;
	int level = 0;

	while (block_iter_prev_line(&bi)) {
		fetch_this_line(&bi, &lr);
		if (regexp_match_nosub(spattern, lr.line, lr.size)) {
			if (level++ == 0) {
				struct indent_info info;
				get_indent_info(lr.line, lr.size, &info);
				return info.width;
			}
		}
		if (regexp_match_nosub(epattern, lr.line, lr.size))
			level--;
	}
	return -1;
}

void unselect(void)
{
	if (view->selection) {
		view->selection = SELECT_NONE;
		mark_all_lines_changed(buffer);
	}
}

static void record_copy(char *buf, long len, bool is_lines)
{
	if (copy_buf)
		free(copy_buf);
	copy_buf = buf;
	copy_len = len;
	copy_is_lines = is_lines;
}

void cut(long len, bool is_lines)
{
	if (len) {
		char *buf = block_iter_get_bytes(&view->cursor, len);
		record_copy(buf, len, is_lines);
		buffer_delete_bytes(len);
	}
}

void copy(long len, bool is_lines)
{
	if (len) {
		char *buf = block_iter_get_bytes(&view->cursor, len);
		record_copy(buf, len, is_lines);
	}
}

void insert_text(const char *text, long size)
{
	long del_count = 0;

	if (view->selection) {
		del_count = prepare_selection(view);
		unselect();
	}
	buffer_replace_bytes(del_count, text, size);
	block_iter_skip_bytes(&view->cursor, size);
}

void paste(void)
{
	long del_count = 0;

	if (!copy_buf)
		return;

	if (view->selection) {
		del_count = prepare_selection(view);
		unselect();
	}

	if (copy_is_lines) {
		int x = view_get_preferred_x(view);
		if (!del_count)
			block_iter_eat_line(&view->cursor);
		buffer_replace_bytes(del_count, copy_buf, copy_len);

		// try to keep cursor column
		move_to_preferred_x(x);
		// new preferred_x
		view_reset_preferred_x(view);
	} else {
		buffer_replace_bytes(del_count, copy_buf, copy_len);
	}
}

void delete_ch(void)
{
	unsigned int u;
	long size = 0;

	if (view->selection) {
		size = prepare_selection(view);
		unselect();
	} else {
		begin_change(CHANGE_MERGE_DELETE);
		if (buffer->options.emulate_tab)
			size = get_indent_level_bytes_right();
		if (size == 0)
			size = buffer_get_char(&view->cursor, &u);
	}
	buffer_delete_bytes(size);
}

void erase(void)
{
	unsigned int u;
	long size = 0;

	if (view->selection) {
		size = prepare_selection(view);
		unselect();
	} else {
		begin_change(CHANGE_MERGE_ERASE);
		if (buffer->options.emulate_tab) {
			size = get_indent_level_bytes_left();
			block_iter_back_bytes(&view->cursor, size);
		}
		if (size == 0)
			size = buffer_prev_char(&view->cursor, &u);
	}
	buffer_erase_bytes(size);
}

// goto beginning of whitespace (tabs and spaces) under cursor and
// return number of whitespace bytes after cursor after moving cursor
static long goto_beginning_of_whitespace(void)
{
	struct block_iter bi = view->cursor;
	long count = 0;
	unsigned int u;

	// count spaces and tabs at or after cursor
	while (buffer_next_char(&bi, &u)) {
		if (u != '\t' && u != ' ')
			break;
		count++;
	}

	// count spaces and tabs before cursor
	while (buffer_prev_char(&view->cursor, &u)) {
		if (u != '\t' && u != ' ') {
			buffer_next_char(&view->cursor, &u);
			break;
		}
		count++;
	}
	return count;
}

static bool ws_only(struct lineref *lr)
{
	long i;
	for (i = 0; i < lr->size; i++) {
		char ch = lr->line[i];
		if (ch != ' ' && ch != '\t')
			return false;
	}
	return true;
}

// non-empty line can be used to determine size of indentation for the next line
static bool find_non_empty_line_bwd(struct block_iter *bi)
{
	block_iter_bol(bi);
	do {
		struct lineref lr;
		fill_line_ref(bi, &lr);
		if (!ws_only(&lr))
			return true;
	} while (block_iter_prev_line(bi));
	return false;
}

static void insert_nl(void)
{
	long del_count = 0;
	long ins_count = 1;
	char *ins = NULL;

	// prepare deleted text (selection or whitespace around cursor)
	if (view->selection) {
		del_count = prepare_selection(view);
		unselect();
	} else {
		// trim whitespace around cursor
		del_count = goto_beginning_of_whitespace();
	}

	// prepare inserted indentation
	if (buffer->options.auto_indent) {
		// current line will be split at cursor position
		struct block_iter bi = view->cursor;
		long len = block_iter_bol(&bi);
		struct lineref lr;

		fill_line_ref(&bi, &lr);
		lr.size = len;
		if (ws_only(&lr)) {
			// This line is (or will become) white space only.
			// Find previous non whitespace only line.
			if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
				fill_line_ref(&bi, &lr);
				ins = get_indent_for_next_line(lr.line, lr.size);
			}
		} else {
			ins = get_indent_for_next_line(lr.line, lr.size);
		}
	}

	begin_change(CHANGE_MERGE_NONE);
	if (ins) {
		// add newline before indent
		ins_count = strlen(ins);
		memmove(ins + 1, ins, ins_count);
		ins[0] = '\n';
		ins_count++;

		buffer_replace_bytes(del_count, ins, ins_count);
		free(ins);
	} else {
		buffer_replace_bytes(del_count, "\n", ins_count);
	}
	end_change();

	// move after inserted text
	block_iter_skip_bytes(&view->cursor, ins_count);
}

void insert_ch(unsigned int ch)
{
	long del_count = 0;
	long ins_count = 0;
	char *ins;

	if (ch == '\n') {
		insert_nl();
		return;
	}

	ins = xmalloc(8);
	if (view->selection) {
		// prepare deleted text (selection)
		del_count = prepare_selection(view);
		unselect();
	} else if (ch == '}' && buffer->options.auto_indent && buffer->options.brace_indent) {
		struct block_iter bi = view->cursor;
		struct lineref curlr;

		block_iter_bol(&bi);
		fill_line_ref(&bi, &curlr);
		if (ws_only(&curlr)) {
			int width = get_indent_of_matching_brace();

			if (width >= 0) {
				// replace current (ws only) line with some indent + '}'
				block_iter_bol(&view->cursor);
				del_count = curlr.size;
				if (width) {
					free(ins);
					ins = make_indent(width);
					ins_count = strlen(ins);
					// '}' will be replace the terminating NUL
				}
			}
		}
	}

	// prepare inserted text
	if (ch == '\t' && buffer->options.expand_tab) {
		ins_count = buffer->options.indent_width;
		memset(ins, ' ', ins_count);
	} else {
		u_set_char_raw(ins, &ins_count, ch);
	}

	// record change
	if (del_count) {
		begin_change(CHANGE_MERGE_NONE);
	} else {
		begin_change(CHANGE_MERGE_INSERT);
	}
	buffer_replace_bytes(del_count, ins, ins_count);
	end_change();

	// move after inserted text
	block_iter_skip_bytes(&view->cursor, ins_count);

	free(ins);
}

static void join_selection(void)
{
	long count = prepare_selection(view);
	long len = 0, join = 0;
	struct block_iter bi;
	unsigned int ch = 0;

	unselect();
	bi = view->cursor;

	begin_change_chain();
	while (count > 0) {
		if (!len)
			view->cursor = bi;

		count -= buffer_next_char(&bi, &ch);
		if (ch == '\t' || ch == ' ') {
			len++;
		} else if (ch == '\n') {
			len++;
			join++;
		} else {
			if (join) {
				buffer_replace_bytes(len, " ", 1);
				/* skip the space we inserted and the char we read last */
				buffer_next_char(&view->cursor, &ch);
				buffer_next_char(&view->cursor, &ch);
				bi = view->cursor;
			}
			len = 0;
			join = 0;
		}
	}

	/* don't replace last \n which is at end of the selection */
	if (join && ch == '\n') {
		join--;
		len--;
	}

	if (join) {
		if (ch == '\n') {
			/* don't add space to end of line */
			buffer_delete_bytes(len);
		} else {
			buffer_replace_bytes(len, " ", 1);
		}
	}
	end_change_chain();
}

void join_lines(void)
{
	struct block_iter next, bi = view->cursor;
	int count;
	unsigned int u;

	if (view->selection) {
		join_selection();
		return;
	}

	if (!block_iter_next_line(&bi))
		return;
	if (block_iter_is_eof(&bi))
		return;

	next = bi;
	buffer_prev_char(&bi, &u);
	count = 1;
	while (buffer_prev_char(&bi, &u)) {
		if (u != '\t' && u != ' ') {
			buffer_next_char(&bi, &u);
			break;
		}
		count++;
	}
	while (buffer_next_char(&next, &u)) {
		if (u != '\t' && u != ' ')
			break;
		count++;
	}

	view->cursor = bi;
	if (u == '\n') {
		buffer_delete_bytes(count);
	} else {
		buffer_replace_bytes(count, " ", 1);
	}
}

static void shift_right(int nr_lines, int count)
{
	int i, indent_size;
	char *indent;

	indent = alloc_indent(count, &indent_size);
	i = 0;
	while (1) {
		struct indent_info info;
		struct lineref lr;

		fetch_this_line(&view->cursor, &lr);
		get_indent_info(lr.line, lr.size, &info);
		if (info.wsonly) {
			if (info.bytes) {
				// remove indentation
				buffer_delete_bytes(info.bytes);
			}
		} else if (info.sane) {
			// insert whitespace
			buffer_insert_bytes(indent, indent_size);
		} else {
			// replace whole indentation with sane one
			int size;
			char *buf = alloc_indent(info.level + count, &size);
			buffer_replace_bytes(info.bytes, buf, size);
		}
		if (++i == nr_lines)
			break;
		block_iter_eat_line(&view->cursor);
	}
	free(indent);
}

static void shift_left(int nr_lines, int count)
{
	int i;

	i = 0;
	while (1) {
		struct indent_info info;
		struct lineref lr;

		fetch_this_line(&view->cursor, &lr);
		get_indent_info(lr.line, lr.size, &info);
		if (info.wsonly) {
			if (info.bytes) {
				// remove indentation
				buffer_delete_bytes(info.bytes);
			}
		} else if (info.level && info.sane) {
			int n = count;

			if (n > info.level)
				n = info.level;
			if (use_spaces_for_indent())
				n *= buffer->options.indent_width;
			buffer_delete_bytes(n);
		} else if (info.bytes) {
			// replace whole indentation with sane one
			if (info.level > count) {
				int size;
				char *buf = alloc_indent(info.level - count, &size);
				buffer_replace_bytes(info.bytes, buf, size);
			} else {
				buffer_delete_bytes(info.bytes);
			}
		}
		if (++i == nr_lines)
			break;
		block_iter_eat_line(&view->cursor);
	}
}

void shift_lines(int count)
{
	int nr_lines = 1;
	struct selection_info info;
	int x = view_get_preferred_x(view) + buffer->options.indent_width * count;

	if (x < 0)
		x = 0;

	if (view->selection) {
		view->selection = SELECT_LINES;
		init_selection(view, &info);
		view->cursor = info.si;
		nr_lines = get_nr_selected_lines(&info);
	}

	begin_change_chain();
	block_iter_bol(&view->cursor);
	if (count > 0)
		shift_right(nr_lines, count);
	else
		shift_left(nr_lines, -count);
	end_change_chain();

	if (view->selection) {
		if (info.swapped) {
			// cursor should be at beginning of selection
			block_iter_bol(&view->cursor);
			view->sel_so = block_iter_get_offset(&view->cursor);
			while (--nr_lines)
				block_iter_prev_line(&view->cursor);
		} else {
			struct block_iter save = view->cursor;
			while (--nr_lines)
				block_iter_prev_line(&view->cursor);
			view->sel_so = block_iter_get_offset(&view->cursor);
			view->cursor = save;
		}
	}
	move_to_preferred_x(x);
}

void clear_lines(void)
{
	long del_count = 0, ins_count = 0;
	char *indent = NULL;

	if (buffer->options.auto_indent) {
		struct block_iter bi = view->cursor;

		if (block_iter_prev_line(&bi) && find_non_empty_line_bwd(&bi)) {
			struct lineref lr;
			fill_line_ref(&bi, &lr);
			indent = get_indent_for_next_line(lr.line, lr.size);
		}
	}

	if (view->selection) {
		view->selection = SELECT_LINES;
		del_count = prepare_selection(view);
		unselect();

		// don't delete last newline
		if (del_count)
			del_count--;
	} else {
		block_iter_eol(&view->cursor);
		del_count = block_iter_bol(&view->cursor);
	}

	if (!indent && !del_count)
		return;

	if (indent)
		ins_count = strlen(indent);
	buffer_replace_bytes(del_count, indent, ins_count);
	block_iter_skip_bytes(&view->cursor, ins_count);
}

void new_line(void)
{
	long ins_count = 1;
	char *ins = NULL;

	block_iter_eol(&view->cursor);

	if (buffer->options.auto_indent) {
		struct block_iter bi = view->cursor;

		if (find_non_empty_line_bwd(&bi)) {
			struct lineref lr;
			fill_line_ref(&bi, &lr);
			ins = get_indent_for_next_line(lr.line, lr.size);
		}
	}

	if (ins) {
		ins_count = strlen(ins);
		memmove(ins + 1, ins, ins_count);
		ins[0] = '\n';
		ins_count++;
		buffer_insert_bytes(ins, ins_count);
		free(ins);
	} else {
		buffer_insert_bytes("\n", 1);
	}

	block_iter_skip_bytes(&view->cursor, ins_count);
}

static void add_word(struct paragraph_formatter *pf, const char *word, int len)
{
	long i = 0;
	int word_width = 0;

	while (i < len)
		word_width += u_char_width(u_get_char(word, len, &i));

	if (pf->cur_width && pf->cur_width + 1 + word_width > pf->text_width) {
		gbuf_add_ch(&pf->buf, '\n');
		pf->cur_width = 0;
	}

	if (pf->cur_width == 0) {
		gbuf_add_buf(&pf->buf, pf->indent, pf->indent_len);
		pf->cur_width = pf->indent_width;
	} else {
		gbuf_add_ch(&pf->buf, ' ');
		pf->cur_width++;
	}

	gbuf_add_buf(&pf->buf, word, len);
	pf->cur_width += word_width;
}

static bool is_paragraph_separator(const char *line, long size)
{
	return regexp_match_nosub("^\\s*(/\\*|\\*/)?\\s*$", line, size);
}

static int get_indent_width(const char *line, long size)
{
	struct indent_info info;

	get_indent_info(line, size, &info);
	return info.width;
}

static bool in_paragraph(const char *line, long size, int indent_width)
{
	if (get_indent_width(line, size) != indent_width)
		return false;
	return !is_paragraph_separator(line, size);
}

static unsigned int paragraph_size(void)
{
	struct block_iter bi = view->cursor;
	struct lineref lr;
	unsigned int size;
	int indent_width;

	block_iter_bol(&bi);
	fill_line_ref(&bi, &lr);
	if (is_paragraph_separator(lr.line, lr.size)) {
		// not in paragraph
		return 0;
	}
	indent_width = get_indent_width(lr.line, lr.size);

	// goto beginning of paragraph
	while (block_iter_prev_line(&bi)) {
		fill_line_ref(&bi, &lr);
		if (!in_paragraph(lr.line, lr.size, indent_width)) {
			block_iter_eat_line(&bi);
			break;
		}
	}
	view->cursor = bi;

	// get size of paragraph
	size = 0;
	do {
		long bytes = block_iter_eat_line(&bi);

		if (!bytes)
			break;

		size += bytes;
		fill_line_ref(&bi, &lr);
	} while (in_paragraph(lr.line, lr.size, indent_width));
	return size;
}

void format_paragraph(int text_width)
{
	struct paragraph_formatter pf;
	long len, i;
	int indent_width;
	char *sel;

	if (view->selection) {
		view->selection = SELECT_LINES;
		len = prepare_selection(view);
	} else {
		len = paragraph_size();
	}
	if (!len)
		return;

	sel = block_iter_get_bytes(&view->cursor, len);
	indent_width = get_indent_width(sel, len);

	gbuf_init(&pf.buf);
	pf.indent = make_indent(indent_width);
	pf.indent_len = pf.indent ? strlen(pf.indent) : 0;
	pf.indent_width = indent_width;
	pf.cur_width = 0;
	pf.text_width = text_width;

	i = 0;
	while (1) {
		long start, tmp;

		while (i < len) {
			tmp = i;
			if (!u_is_space(u_get_char(sel, len, &tmp)))
				break;
			i = tmp;
		}
		if (i == len)
			break;

		start = i;
		while (i < len) {
			tmp = i;
			if (u_is_space(u_get_char(sel, len, &tmp)))
				break;
			i = tmp;
		}

		add_word(&pf, sel + start, i - start);
	}

	if (pf.buf.len)
		gbuf_add_ch(&pf.buf, '\n');
	buffer_replace_bytes(len, pf.buf.buffer, pf.buf.len);
	if (pf.buf.len)
		block_iter_skip_bytes(&view->cursor, pf.buf.len - 1);
	gbuf_free(&pf.buf);
	free(pf.indent);
	free(sel);

	unselect();
}

void change_case(int mode)
{
	bool was_selecting = false;
	bool move = true;
	long text_len, i;
	char *src;
	GBUF(dst);

	if (view->selection) {
		struct selection_info info;

		init_selection(view, &info);
		view->cursor = info.si;
		text_len = info.eo - info.so;
		unselect();
		was_selecting = true;
		move = !info.swapped;
	} else {
		unsigned int u;

		if (!buffer_get_char(&view->cursor, &u))
			return;

		text_len = u_char_size(u);
	}

	src = block_iter_get_bytes(&view->cursor, text_len);
	i = 0;
	while (i < text_len) {
		unsigned int u = u_get_char(src, text_len, &i);

		switch (mode) {
		case 't':
			if (iswupper(u))
				u = towlower(u);
			else
				u = towupper(u);
			break;
		case 'l':
			u = towlower(u);
			break;
		case 'u':
			u = towupper(u);
			break;
		}
		gbuf_add_ch(&dst, u);
	}

	buffer_replace_bytes(text_len, dst.buffer, dst.len);
	free(src);

	if (move && dst.len > 0) {
		if (was_selecting) {
			// move cursor back to where it was
			long idx = dst.len;
			u_prev_char(dst.buffer, &idx);
			block_iter_skip_bytes(&view->cursor, idx);
		} else {
			block_iter_skip_bytes(&view->cursor, dst.len);
		}
	}

	gbuf_free(&dst);
}
