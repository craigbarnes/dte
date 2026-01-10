#include "terminal.h"
#include "color.h"
#include "output.h"
#include "parse.h"

void term_init(Terminal *term, const char *name, const char *colorterm)
{
    TermFeatureFlags features = term_get_features(name, colorterm);
    term->features = features;
    term->width = 80;
    term->height = 24;

    term->ncv_attributes =
        (features & TFLAG_NCV_UNDERLINE) ? ATTR_UNDERLINE : 0
        | (features & TFLAG_NCV_DIM) ? ATTR_DIM : 0
        | (features & TFLAG_NCV_REVERSE) ? ATTR_REVERSE : 0
    ;
}

void term_enable_private_modes(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = term->features;

    // Note that changes to some of the sequences below may require
    // corresponding updates to handle_query_reply()

    if (features & TFLAG_META_ESC) {
        term_put_literal(obuf, "\033[?1036h"); // DECSET 1036 (metaSendsEscape)
    }
    if (features & TFLAG_ALT_ESC) {
        term_put_literal(obuf, "\033[?1039h"); // DECSET 1039 (altSendsEscape)
    }

    if (features & TFLAG_KITTY_KEYBOARD) {
        // https://sw.kovidgoyal.net/kitty/keyboard-protocol/#progressive-enhancement
        term_put_literal(obuf, "\033[>5u");
    } else if (features & TFLAG_MODIFY_OTHER_KEYS) {
        // Try to use "modifyOtherKeys" mode (level 2 or 1)
        term_put_literal(obuf, "\033[>4;1m\033[>4;2m");
    }

    // Try to enable bracketed paste mode. This is done unconditionally,
    // since it should be ignored by terminals that don't recognize it
    // and we really want to enable it for terminals that support it but
    // are spoofing $TERM for whatever reason.
    term_put_literal(obuf, "\033[?2004s\033[?2004h");
}

void term_restore_private_modes(Terminal *term)
{
    TermOutputBuffer *obuf = &term->obuf;
    TermFeatureFlags features = term->features;
    if (features & TFLAG_META_ESC) {
        term_put_literal(obuf, "\033[?1036l"); // DECRST 1036 (metaSendsEscape)
    }
    if (features & TFLAG_ALT_ESC) {
        term_put_literal(obuf, "\033[?1039l"); // DECRST 1039 (altSendsEscape)
    }
    if (features & TFLAG_KITTY_KEYBOARD) {
        term_put_literal(obuf, "\033[<u");
    } else if (features & TFLAG_MODIFY_OTHER_KEYS) {
        term_put_literal(obuf, "\033[>4m");
    }
    term_put_literal(obuf, "\033[?2004l\033[?2004r");
}

void term_restore_cursor_style(Terminal *term)
{
    // TODO: Query the cursor style at startup (using DECRQSS DECSCUSR)
    // and restore the value provided in the reply (if any), instead
    // of using CURSOR_DEFAULT (which basically amounts to using the
    // so-called "DECSCUSR 0 hack")
    static const TermCursorStyle reset = {
        .type = CURSOR_DEFAULT,
        .color = COLOR_DEFAULT,
    };

    term_set_cursor_style(term, reset);
}
