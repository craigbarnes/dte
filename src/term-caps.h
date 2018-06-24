#ifndef TERM_CAPS_H
#define TERM_CAPS_H

#include <stdbool.h>
#include <sys/types.h>
#include "key.h"

typedef struct {
    const char *clear_to_eol;
    const char *keypad_off;
    const char *keypad_on;
    const char *cup_mode_off;
    const char *cup_mode_on;
    const char *show_cursor;
    const char *hide_cursor;
    const char *save_title;
    const char *restore_title;
    const char *set_title_begin;
    const char *set_title_end;
} TermControlCodes;

typedef struct {
    bool back_color_erase;
    int max_colors;
    int width;
    int height;
    unsigned short attributes;
    unsigned short ncv_attributes;
    const TermControlCodes *control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, Key *key);
} TerminalInfo;

extern TerminalInfo terminal;

void term_init(void);

#endif
