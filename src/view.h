#ifndef VIEW_H
#define VIEW_H

#include <limits.h>
#include <stdbool.h>
#include <sys/types.h>
#include "block-iter.h"
#include "util/macros.h"
#include "util/string-view.h"

typedef enum {
    SELECT_NONE,
    SELECT_CHARS,
    SELECT_LINES,
} SelectionType;

// A view into a Buffer, with its own cursor position and selection.
// Visually speaking, each tab in a Window corresponds to a View.
typedef struct View {
    struct Buffer *buffer;
    struct Window *window;
    BlockIter cursor;
    long cx, cy; // Cursor position
    long cx_display; // Visual cursor x (char widths: wide 2, tab 1-8, control 2, invalid char 4)
    long cx_char; // Cursor x in characters (invalid UTF-8 character (byte) is 1 char)
    long vx, vy; // Top left corner
    long preferred_x; // Preferred cursor x (preferred value for cx_display)
    int tt_width; // Tab title width
    int tt_truncated_width;
    bool center_on_scroll; // Center view to cursor if scrolled
    bool force_center; // Force centering view to cursor

    SelectionType selection;
    SelectionType select_mode;
    ssize_t sel_so; // Cursor offset when selection was started
    ssize_t sel_eo; // See `SEL_EO_RECALC` below

    // Used to save cursor state when multiple views share same buffer
    bool restore_cursor;
    size_t saved_cursor_offset;
} View;

// If View::sel_eo is set to this value it means the offset must
// be calculated from the cursor iterator. Otherwise the offset
// is precalculated and may not be the same as the cursor position
// (see search/replace code).
#define SEL_EO_RECALC SSIZE_MAX

static inline void view_reset_preferred_x(View *view)
{
    view->preferred_x = -1;
}

void view_update_cursor_y(View *view) NONNULL_ARGS;
void view_update_cursor_x(View *view) NONNULL_ARGS;
void view_update(View *view, unsigned int scroll_margin) NONNULL_ARGS;
long view_get_preferred_x(View *view) NONNULL_ARGS;
bool view_can_close(const View *view) NONNULL_ARGS;
StringView view_do_get_word_under_cursor(const View *view, size_t *offset_in_line) NONNULL_ARGS;
StringView view_get_word_under_cursor(const View *view) NONNULL_ARGS;

#endif
