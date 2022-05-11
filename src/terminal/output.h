#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "color.h"
#include "terminal.h"
#include "util/macros.h"
#include "util/unicode.h"

#define term_add_literal(buf, s) term_add_bytes(buf, s, STRLEN(s))

static inline size_t obuf_avail(TermOutputBuffer *obuf)
{
    return TERM_OUTBUF_SIZE - obuf->count;
}

void term_output_init(TermOutputBuffer *obuf);
void term_output_free(TermOutputBuffer *obuf);
void term_output_reset(Terminal *term, size_t start_x, size_t width, size_t scroll_x);
void term_add_byte(TermOutputBuffer *obuf, char ch);
void term_add_bytes(TermOutputBuffer *obuf, const char *str, size_t count);
void term_set_bytes(Terminal *term, char ch, size_t count);
void term_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count);
void term_add_str(TermOutputBuffer *obuf, const char *str);
void term_add_uint(TermOutputBuffer *obuf, unsigned int x);
void term_use_alt_screen_buffer(Terminal *term);
void term_use_normal_screen_buffer(Terminal *term);
void term_hide_cursor(Terminal *term);
void term_show_cursor(Terminal *term);
void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y);
void term_save_title(Terminal *term);
void term_restore_title(Terminal *term);
void term_clear_eol(Terminal *term);
void term_clear_screen(TermOutputBuffer *obuf);
void term_output_flush(TermOutputBuffer *obuf) NOINLINE;
bool term_put_char(TermOutputBuffer *obuf, CodePoint u);
void term_set_color(Terminal *term, const TermColor *color);
void term_set_cursor_style(Terminal *term, TermCursorStyle style);

#endif
