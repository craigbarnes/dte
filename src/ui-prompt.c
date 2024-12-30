#include "ui.h"
#include "error.h"
#include "signals.h"
#include "terminal/input.h"
#include "terminal/paste.h"

/*
 * Read input until a key is pressed that matches one of the (ASCII)
 * characters in `choices` or one of the pre-defined keys (Enter, Esc
 * Ctrl+c, Ctrl+g). The default choice should be given as choices[0]
 * and pressing Enter is equivalent to pressing that key.
 *
 * Note that this is used to "hijack" input handling at the point it's
 * called, as opposed to using the normal input handling in main_loop().
 * This is so that e.g. `replace -c` can prompt at the appropriate point
 * in the following string of commands:
 *
 *  open -t; repeat 3 insert "xyz\n"; replace -c y _; down; insert "xyz\n"
 *
 * ...and execution can then continue with the next command, without
 * involving main_loop() at all.
 */
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
    case KEY_ESCAPE:
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

void show_dialog(Terminal *term, const StyleMap *styles, const char *question)
{
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
    TermOutputBuffer *obuf = &term->obuf;

    // The "underline" and "strikethrough" attributes should only apply
    // to the text, not the whole dialog background:
    TermStyle text_style = styles->builtin[BSE_DIALOG];
    TermStyle dialog_style = text_style;
    dialog_style.attr &= ~(ATTR_UNDERLINE | ATTR_STRIKETHROUGH);
    set_style(term, styles, &dialog_style);

    for (unsigned int y = top; y < bot; y++) {
        term_output_reset(term, x, width, 0);
        term_move_cursor(obuf, x, y);
        if (y == mid) {
            term_set_bytes(term, ' ', (width - question_width) / 2);
            set_style(term, styles, &text_style);
            term_put_str(obuf, question);
            set_style(term, styles, &dialog_style);
        }
        term_clear_eol(term);
    }
}

char dialog_prompt(EditorState *e, const char *question, const char *choices)
{
    BUG_ON(e->flags & EFLAG_HEADLESS);

    const ScreenState dummyval = {.id = 0};
    info_msg(e->err, "%s", question);
    e->screen_update |= UPDATE_ALL | UPDATE_DIALOG;
    update_screen(e, &dummyval);

    Terminal *term = &e->terminal;
    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;
    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (resized) {
            e->screen_update |= UPDATE_DIALOG;
            ui_resize(e);
        }
    }

    clear_error(e->err);
    e->screen_update |= UPDATE_ALL;
    return (choice >= 'a') ? choice : 0;
}

char status_prompt(EditorState *e, const char *question, const char *choices)
{
    BUG_ON(e->flags & EFLAG_HEADLESS);

    const ScreenState dummyval = {.id = 0};
    info_msg(e->err, "%s", question);
    update_screen(e, &dummyval);

    Terminal *term = &e->terminal;
    unsigned int esc_timeout = e->options.esc_timeout;
    char choice;

    while ((choice = get_choice(term, choices, esc_timeout)) == 0) {
        if (resized) {
            ui_resize(e);
        }
    }

    clear_error(e->err);
    return (choice >= 'a') ? choice : 0;
}
