#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include "color.h"
#include "util/macros.h"
#include "util/string-view.h"
#include "util/unicode.h"

typedef struct {
    char buf[8192];
    size_t count;

    // Number of characters scrolled (x direction)
    size_t scroll_x;

    // Current x position (tab 1-8, double-width 2, invalid UTF-8 byte 4)
    // if smaller than scroll_x printed characters are not visible
    size_t x;

    size_t width;

    size_t tab_width;
    enum {
        TAB_NORMAL,
        TAB_SPECIAL,
        TAB_CONTROL,
    } tab;
    bool can_clear;

    TermColor color;
} TermOutputBuffer;

extern TermOutputBuffer obuf;

#define term_add_literal(s) term_add_bytes(s, STRLEN(s))

void term_output_reset(size_t start_x, size_t width, size_t scroll_x);
void term_add_byte(char ch);
void term_add_bytes(const char *str, size_t count);
void term_set_bytes(char ch, size_t count);
void term_repeat_byte(char ch, size_t count);
void term_add_string_view(StringView sv);
void term_add_str(const char *str);
size_t term_xnprintf(size_t maxlen, const char *format, ...) PRINTF(2);
void term_hide_cursor(void);
void term_show_cursor(void);
void term_move_cursor(unsigned int x, unsigned int y);
void term_clear_eol(void);
void term_output_flush(void);
bool term_put_char(CodePoint u);

#endif
