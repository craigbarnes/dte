#include "screen.h"
#include "signals.h"
#include "terminal/input.h"

static char get_choice(Terminal *term, const char *choices, unsigned int esc_timeout)
{
    KeyCode key = term_read_key(term, esc_timeout);
    if (key == KEY_NONE) {
        return 0;
    }

    switch (key) {
    case KEY_BRACKETED_PASTE:
    case KEY_DETECTED_PASTE:
        term_discard_paste(&term->ibuf, key == KEY_BRACKETED_PASTE);
        return 0;
    case MOD_CTRL | 'c':
    case MOD_CTRL | 'g':
    case MOD_CTRL | '[':
        return 0x18; // Cancel
    case KEY_ENTER:
        return choices[0]; // Default
    }

    if (key < 128) {
        char ch = ascii_tolower(key);
        if (strchr(choices, ch)) {
            return ch;
        }
    }
    return 0;
}

static void show_dialog (
    EditorState *e,
    const TermColor *text_color,
    const char *question
) {
    Terminal *term = &e->terminal;
    unsigned int question_width = u_str_width(question);
    unsigned int min_width = question_width + 2;
    if (term->height < 12 || term->width < min_width) {
        return;
    }

    unsigned int height = term->height / 4;
    unsigned int mid = term->height / 2;
    unsigned int top = mid - (height / 2);
    unsigned int bot = top + height;
    unsigned int width = MAX(term->width / 2, min_width);
    unsigned int x = (term->width - width) / 2;

    // The "underline" and "strikethrough" attributes should only apply
    // to the text, not the whole dialog background:
    TermColor dialog_color = *text_color;
    TermOutputBuffer *obuf = &term->obuf;
    dialog_color.attr &= ~(ATTR_UNDERLINE | ATTR_STRIKETHROUGH);
    set_color(term, &e->colors, &dialog_color);

    for (unsigned int y = top; y < bot; y++) {
        term_output_reset(term, x, width, 0);
        term_move_cursor(obuf, x, y);
        if (y == mid) {
            term_set_bytes(term, ' ', (width - question_width) / 2);
            set_color(term, &e->colors, text_color);
            term_add_str(obuf, question);
            set_color(term, &e->colors, &dialog_color);
        }
        term_clear_eol(term);
    }
}

char dialog_prompt(EditorState *e, const char *question, const char *choices)
{
    const TermColor *color = &e->colors.builtin[BC_DIALOG];
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;

    normal_update(e);
    term_hide_cursor(term);
    show_dialog(e, color, question);
    show_message(term, &e->colors, question, false);
    term_output_flush(obuf);

    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (!resized) {
            continue;
        }
        ui_resize(e);
        term_hide_cursor(term);
        show_dialog(e, color, question);
        show_message(term, &e->colors, question, false);
        term_output_flush(obuf);
    }

    mark_everything_changed(e);
    return (choice >= 'a') ? choice : 0;
}

char status_prompt(EditorState *e, const char *question, const char *choices)
{
    // update_buffer_windows() assumes these have been called for current view
    view_update_cursor_x(e->view);
    view_update_cursor_y(e->view);
    view_update(e->view, e->options.scroll_margin);

    // Set changed_line_min and changed_line_max before calling update_range()
    mark_all_lines_changed(e->buffer);

    Terminal *term = &e->terminal;
    start_update(term);
    update_term_title(term, e->buffer, e->options.set_window_title);
    update_buffer_windows(e, e->buffer);
    show_message(term, &e->colors, question, false);
    end_update(e);

    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (!resized) {
            continue;
        }
        ui_resize(e);
        term_hide_cursor(term);
        show_message(term, &e->colors, question, false);
        restore_cursor(e);
        term_show_cursor(term);
        term_output_flush(&term->obuf);
    }

    return (choice >= 'a') ? choice : 0;
}
