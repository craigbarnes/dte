#ifndef TERMINAL_OUTPUT_H
#define TERMINAL_OUTPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "color.h"
#include "../util/macros.h"
#include "../util/unicode.h"

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
} OutputBuffer;

extern OutputBuffer obuf;

#define buf_add_literal(s) buf_add_bytes(s, STRLEN(s))

void buf_reset(size_t start_x, size_t width, size_t scroll_x);
void buf_add_bytes(const char *str, size_t count);
void buf_set_bytes(char ch, size_t count);
void buf_repeat_byte(char ch, size_t count);
void buf_add_ch(char ch);
void buf_sprintf(const char *fmt, ...) PRINTF(1);
void buf_add_str(const char *str);
void buf_hide_cursor(void);
void buf_show_cursor(void);
void buf_clear_eol(void);
void buf_flush(void);
bool buf_put_char(CodePoint u);

#endif
