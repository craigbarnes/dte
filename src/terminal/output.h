#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include "color.h"
#include "style.h"
#include "terminal.h"
#include "util/macros.h"
#include "util/unicode.h"

enum {
    // Minimum repeat count for which ECMA-48 REP sequence makes sense.
    // STRLEN("_\033[5b") is 5 bytes and the sequence fills 6 columns.
    // Any repeat count lower than 6 can be done with memset(3) in equal
    // or fewer bytes.
    ECMA48_REP_MIN = 6,

    // Maximum repeat count for which ECMA-48 REP sequence should be used
    // (fits in 15 bits, to avoid issues in terminals using int16_t params).
    ECMA48_REP_MAX = 30000,
};

typedef enum {
    // Indicates that term_set_bytes() wrote 1 byte followed by an ECMA-48
    // (§8.3.103) REP sequence, in order to instruct the terminal to fill
    // a run of cells with the same character
    TERM_SET_BYTES_REP,

    // Indicates that term_set_bytes() simply called memset(3), in order
    // to fill the output buffer (and thus also the terminal) with a run
    // of the same character
    TERM_SET_BYTES_MEMSET,
} TermSetBytesMethod;

enum {
    // Returned by term_clear_eol() to indicate that an ECMA-48 (§8.3.41)
    // EL sequence was used to erase the remainder of the current line,
    // instead of REP or memset(3)
    TERM_CLEAR_EOL_USED_EL = -1,
};

#define term_put_literal(buf, s) term_put_bytes(buf, s, STRLEN(s))

#define TERM_OUTPUT_INIT { \
    .tab_mode = TAB_CONTROL, \
    .tab_width = 8, \
    .style = {.fg = COLOR_DEFAULT, .bg = COLOR_DEFAULT}, \
    .cursor_style = {.type = CURSOR_DEFAULT, .color = COLOR_DEFAULT}, \
}

static inline size_t obuf_avail(TermOutputBuffer *obuf)
{
    return TERM_OUTBUF_SIZE - obuf->count;
}

char *term_output_reserve_space(TermOutputBuffer *obuf, size_t count) NONNULL_ARGS_AND_RETURN WARN_UNUSED_RESULT;
void term_output_reset(Terminal *term, size_t start_x, size_t width, size_t scroll_x) NONNULL_ARGS;
void term_put_byte(TermOutputBuffer *obuf, char ch) NONNULL_ARGS;
void term_put_bytes(TermOutputBuffer *obuf, const char *str, size_t count) NONNULL_ARGS;
TermSetBytesMethod term_set_bytes(Terminal *term, char ch, size_t count) NONNULL_ARGS;
void term_put_str(TermOutputBuffer *obuf, const char *str) NONNULL_ARGS;
void term_put_initial_queries(Terminal *term, unsigned int level) NONNULL_ARGS;
void term_put_level_2_queries(Terminal *term, bool emit_all) NONNULL_ARGS;
void term_put_level_3_queries(Terminal *term, bool emit_all) NONNULL_ARGS;
void term_use_alt_screen_buffer(Terminal *term) NONNULL_ARGS;
void term_use_normal_screen_buffer(Terminal *term) NONNULL_ARGS;
void term_hide_cursor(Terminal *term) NONNULL_ARGS;
void term_show_cursor(Terminal *term) NONNULL_ARGS;
void term_begin_sync_update(Terminal *term) NONNULL_ARGS;
void term_end_sync_update(Terminal *term) NONNULL_ARGS;
void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y) NONNULL_ARGS;
void term_save_title(Terminal *term) NONNULL_ARGS;
void term_restore_title(Terminal *term) NONNULL_ARGS;
void term_restore_and_save_title(Terminal *term) NONNULL_ARGS;
bool term_can_clear_eol_with_el_sequence(const Terminal *term) NONNULL_ARGS;
int term_clear_eol(Terminal *term) NONNULL_ARGS;
void term_clear_screen(TermOutputBuffer *obuf) NONNULL_ARGS;
void term_output_flush(TermOutputBuffer *obuf) NOINLINE NONNULL_ARGS;
bool term_put_char(TermOutputBuffer *obuf, CodePoint u) NONNULL_ARGS;
void term_set_style(Terminal *term, TermStyle style) NONNULL_ARGS;
void term_set_cursor_style(Terminal *term, TermCursorStyle style) NONNULL_ARGS;

#endif
