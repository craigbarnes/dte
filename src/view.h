#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>
#include "block-iter.h"

enum selection {
    SELECT_NONE,
    SELECT_CHARS,
    SELECT_LINES,
};

typedef struct View {
    struct Buffer *buffer;
    struct Window *window;

    BlockIter cursor;

    // Cursor position
    int cx, cy;

    // Visual cursor x (char widths: wide 2, tab 1-8, control 2, invalid char 4)
    int cx_display;

    // Cursor x in characters (invalid UTF-8 character (byte) is one char)
    int cx_char;

    // Top left corner
    int vx, vy;

    // Preferred cursor x (preferred value for cx_display)
    int preferred_x;

    // Tab title
    int tt_width;
    int tt_truncated_width;

    enum selection selection;

    // Cursor offset when selection was started
    long sel_so;

    // If sel_eo is UINT_MAX that means the offset must be calculated from
    // the cursor iterator.  Otherwise the offset is precalculated and may
    // not be same as cursor position (see search/replace code).
    long sel_eo;

    // Center view to cursor if scrolled
    bool center_on_scroll;

    // Force centering view to cursor
    bool force_center;

    // These are used to save cursor state when there are multiple views
    // sharing same buffer.
    bool restore_cursor;
    long saved_cursor_offset;
} View;

static inline void view_reset_preferred_x(View *v)
{
    v->preferred_x = -1;
}

void view_update_cursor_y(View *v);
void view_update_cursor_x(View *v);
void view_update(View *v);
int view_get_preferred_x(View *v);
bool view_can_close(View *v);
char *view_get_word_under_cursor(View *v);

#endif
