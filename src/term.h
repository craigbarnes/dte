#ifndef TERM_H
#define TERM_H

#include <stdbool.h>
#include <stddef.h>
#include "key.h"

enum {
    COLOR_DEFAULT = -1,
    COLOR_BLACK,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_GREY
};

enum {
    ATTR_BOLD = 0x01,
    ATTR_LOW_INTENSITY = 0x02,
    ATTR_ITALIC = 0x04,
    ATTR_UNDERLINE = 0x08,
    ATTR_BLINKING = 0x10,
    ATTR_REVERSE_VIDEO = 0x20,
    ATTR_INVISIBLE_TEXT = 0x40,
    ATTR_KEEP = 0x80,
};

typedef struct {
    short fg;
    short bg;
    unsigned short attr;
} TermColor;

typedef struct {
    const char *clear_to_eol;
    const char *keypad_off;
    const char *keypad_on;
    const char *cup_mode_off;
    const char *cup_mode_on;
    const char *show_cursor;
    const char *hide_cursor;
} TermControlCodes;

typedef struct {
    const char *code;
    Key key;
} TermKeyMap;

typedef struct {
    bool can_bg_color_erase; // Can clear to end of line with bg color set
    int max_colors;
    int width;
    int height;
    const TermControlCodes *control_codes;
    const TermKeyMap *keymap;
    size_t keymap_length;
} TerminalInfo;

extern TerminalInfo terminal;

void term_init(const char *const term);

void term_raw(void);
void term_cooked(void);

bool term_read_key(Key *key);
char *term_read_paste(size_t *size);
void term_discard_paste(void);

int term_get_size(int *w, int *h);

#endif
