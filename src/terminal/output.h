#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "color.h"
#include "util/macros.h"
#include "util/string-view.h"
#include "util/unicode.h"

enum {
    TERM_OUTBUF_SIZE = 8192
};

typedef struct {
    char *buf;
    size_t count;

    // Number of characters scrolled (x direction)
    size_t scroll_x;

    // Current x position (tab 1-8, double-width 2, invalid UTF-8 byte 4)
    // if smaller than scroll_x printed characters are not visible
    size_t x;

    size_t width;

    enum {
        TAB_NORMAL,
        TAB_SPECIAL,
        TAB_CONTROL,
    } tab;

    uint8_t tab_width;
    bool can_clear;

    TermColor color;
} TermOutputBuffer;

#define term_add_literal(buf, s) term_add_bytes(buf, s, STRLEN(s))

static inline size_t obuf_avail(TermOutputBuffer *obuf)
{
    return TERM_OUTBUF_SIZE - obuf->count;
}

void term_output_init(TermOutputBuffer *obuf);
void term_output_free(TermOutputBuffer *obuf);
void term_output_reset(TermOutputBuffer *obuf, size_t start_x, size_t width, size_t scroll_x);
void term_add_byte(TermOutputBuffer *obuf, char ch);
void term_add_bytes(TermOutputBuffer *obuf, const char *str, size_t count);
void term_set_bytes(TermOutputBuffer *obuf, char ch, size_t count);
void term_repeat_byte(TermOutputBuffer *obuf, char ch, size_t count);
void term_add_string_view(TermOutputBuffer *obuf, StringView sv);
void term_add_str(TermOutputBuffer *obuf, const char *str);
void term_add_uint(TermOutputBuffer *obuf, unsigned int x);
void term_hide_cursor(TermOutputBuffer *obuf);
void term_show_cursor(TermOutputBuffer *obuf);
void term_move_cursor(TermOutputBuffer *obuf, unsigned int x, unsigned int y);
void term_save_title(TermOutputBuffer *obuf);
void term_restore_title(TermOutputBuffer *obuf);
void term_clear_eol(TermOutputBuffer *obuf);
void term_output_flush(TermOutputBuffer *obuf);
bool term_put_char(TermOutputBuffer *obuf, CodePoint u);

#endif
