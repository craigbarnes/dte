#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <sys/types.h>
#include "color.h"
#include "key.h"
#include "../util/macros.h"
#include "../util/string-view.h"

typedef struct {
    StringView init;
    StringView deinit;
    StringView reset_colors;
    StringView reset_attrs;
    StringView keypad_off;
    StringView keypad_on;
    StringView cup_mode_off;
    StringView cup_mode_on;
    StringView show_cursor;
    StringView hide_cursor;
} TermControlCodes;

typedef struct {
    bool back_color_erase;
    TermColorCapabilityType color_type;
    int width;
    int height;
    unsigned int ncv_attributes;
    TermControlCodes control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, KeyCode *key);
    void (*put_control_code)(StringView code);
    void (*clear_screen)(void);
    void (*clear_to_eol)(void);
    void (*set_color)(const TermColor *color);
    void (*move_cursor)(int x, int y);
    void (*repeat_byte)(char ch, size_t count);
    void (*raw)(void);
    void (*cooked)(void);
    void (*save_title)(void);
    void (*restore_title)(void);
    void (*set_title)(const char *title);
} Terminal;

extern Terminal terminal;

void term_init(void);

NORETURN COLD PRINTF(1)
void term_init_fail(const char *fmt, ...);

#endif
