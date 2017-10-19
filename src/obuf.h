#ifndef OBUF_H
#define OBUF_H

#include "term.h"
#include "libc.h"

typedef struct {
    char buf[8192];
    size_t count;

    // Number of characters scrolled (x direction)
    unsigned int scroll_x;

    // Current x position (tab 1-8, double-width 2, invalid UTF-8 byte 4)
    // if smaller than scroll_x printed characters are not visible
    unsigned int x;

    unsigned int width;

    unsigned int tab_width;
    enum {
        TAB_NORMAL,
        TAB_SPECIAL,
        TAB_CONTROL,
    } tab;
    bool can_clear;

    struct term_color color;
} OutputBuffer;

extern OutputBuffer obuf;

void buf_reset(unsigned int start_x, unsigned int width, unsigned int scroll_x);
void buf_add_bytes(const char *str, int count);
void buf_set_bytes(char ch, int count);
void buf_add_ch(char ch);
void buf_escape(const char *str);
void buf_add_str(const char *str);
void buf_hide_cursor(void);
void buf_show_cursor(void);
void buf_move_cursor(int x, int y);
void buf_set_color(const struct term_color *color);
void buf_clear_eol(void);
void buf_flush(void);
bool buf_put_char(unsigned int u);

#endif
