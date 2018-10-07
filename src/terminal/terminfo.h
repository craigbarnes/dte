#ifndef TERMINAL_TERMINFO_H
#define TERMINAL_TERMINFO_H

#include <stdbool.h>
#include <sys/types.h>
#include "color.h"
#include "key.h"

typedef struct {
    const char *init;
    const char *deinit;
    const char *reset_colors;
    const char *reset_attrs;
    const char *keypad_off;
    const char *keypad_on;
    const char *cup_mode_off;
    const char *cup_mode_on;
    const char *show_cursor;
    const char *hide_cursor;
} TermControlCodes;

typedef struct {
    bool back_color_erase;
    int max_colors;
    int width;
    int height;
    unsigned short ncv_attributes;
    TermControlCodes control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, KeyCode *key);
    void (*clear_to_eol)(void);
    void (*set_color)(const TermColor *color);
    void (*move_cursor)(int x, int y);
    void (*repeat_byte)(char ch, size_t count);
    void (*raw)(void);
    void (*cooked)(void);
    void (*save_title)(void);
    void (*restore_title)(void);
    void (*set_title)(const char *title);
} TerminalInfo;

extern TerminalInfo terminal;

void term_init(void);

#endif
