#include "ui.h"
#include "error.h"
#include "signals.h"
#include "terminal/input.h"
#include "terminal/paste.h"

static char get_choice(Terminal *term, const char *choices, unsigned int esc_timeout)
{
    KeyCode key = term_read_input(term, esc_timeout);

    switch (key) {
    case KEY_NONE:
        return 0;
    case KEYCODE_BRACKETED_PASTE:
    case KEYCODE_DETECTED_PASTE:
        term_discard_paste(&term->ibuf, key == KEYCODE_BRACKETED_PASTE);
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
    Terminal *term,
    const StyleMap *styles,
    const TermStyle *text_style,
    const char *question
) {
    unsigned int question_width = u_str_width(question);
    unsigned int min_width = question_width + 2;
    if (term->height < 9 || term->width < min_width) {
        return;
    }

    unsigned int height = (term->height / 4) | 1;
    unsigned int mid = term->height / 2;
    unsigned int top = mid - (height / 2);
    unsigned int bot = top + height;
    unsigned int width = MAX(term->width / 2, min_width);
    unsigned int x = (term->width - width) / 2;

    // The "underline" and "strikethrough" attributes should only apply
    // to the text, not the whole dialog background:
    TermStyle dialog_style = *text_style;
    TermOutputBuffer *obuf = &term->obuf;
    dialog_style.attr &= ~(ATTR_UNDERLINE | ATTR_STRIKETHROUGH);
    set_style(term, styles, &dialog_style);

    for (unsigned int y = top; y < bot; y++) {
        term_output_reset(term, x, width, 0);
        term_move_cursor(obuf, x, y);
        if (y == mid) {
            term_set_bytes(term, ' ', (width - question_width) / 2);
            set_style(term, styles, text_style);
            term_put_str(obuf, question);
            set_style(term, styles, &dialog_style);
        }
        term_clear_eol(term);
    }
}

char dialog_prompt(EditorState *e, const char *question, const char *choices)
{
    const StyleMap *styles = &e->styles;
    const TermStyle *style = &styles->builtin[BSE_DIALOG];
    Terminal *term = &e->terminal;
    TermOutputBuffer *obuf = &term->obuf;

    start_update(term);
    update_term_title(term, e->buffer, e->options.set_window_title);
    update_all_windows(e);
    show_dialog(term, styles, style, question);
    show_message(term, styles, question, false);
    term_end_sync_update(term);
    term_output_flush(obuf);

    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (!resized) {
            continue;
        }
        resized = 0;
        update_screen_size(&e->terminal, e->root_frame);
        term_begin_sync_update(term);
        update_all_windows(e);
        show_dialog(term, styles, style, question);
        show_message(term, styles, question, false);
        term_end_sync_update(term);
        term_output_flush(obuf);
    }

    e->screen_update |= UPDATE_ALL;
    return (choice >= 'a') ? choice : 0;
}

char status_prompt(EditorState *e, const char *question, const char *choices)
{
    const ScreenState dummyval = {.id = 0};
    e->screen_update |= UPDATE_CURRENT_BUFFER | UPDATE_TERM_TITLE;
    info_msg("%s", question);
    update_screen(e, &dummyval);

    Terminal *term = &e->terminal;
    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;

    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (resized) {
            ui_resize(e);
        }
    }

    clear_error();
    return (choice >= 'a') ? choice : 0;
}
