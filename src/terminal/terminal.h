#ifndef TERMINAL_TERMINAL_H
#define TERMINAL_TERMINAL_H

#include <stdbool.h>
#include <sys/types.h>
#include "color.h"
#include "key.h"
#include "output.h"
#include "util/macros.h"
#include "util/string-view.h"

typedef struct {
    StringView init;
    StringView deinit;
    StringView keypad_off;
    StringView keypad_on;
    StringView cup_mode_off;
    StringView cup_mode_on;
    StringView show_cursor;
    StringView hide_cursor;
    StringView save_title;
    StringView restore_title;
    StringView set_title_begin;
    StringView set_title_end;
} TermControlCodes;

typedef struct {
    bool back_color_erase;
    TermColorCapabilityType color_type;
    unsigned int width;
    unsigned int height;
    unsigned int ncv_attributes;
    TermControlCodes control_codes;
    ssize_t (*parse_key_sequence)(const char *buf, size_t length, KeyCode *key);
    void (*set_color)(TermOutputBuffer *obuf, const TermColor *color);
    void (*repeat_byte)(TermOutputBuffer *obuf, char ch, size_t count);
    bool (*copy_text)(TermOutputBuffer *obuf, const char *text, size_t len, bool clipboard, bool primary);
} Terminal;

extern Terminal terminal;

void term_init(const char *term) NONNULL_ARGS;

#endif
