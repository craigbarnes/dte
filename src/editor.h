#ifndef EDITOR_H
#define EDITOR_H

#include "libc.h"
#include "cmdline.h"

typedef enum {
    EDITOR_INITIALIZING,
    EDITOR_RUNNING,
    EDITOR_EXITING,
} EditorStatus;

typedef enum {
    INPUT_NORMAL,
    INPUT_COMMAND,
    INPUT_SEARCH,
    INPUT_GIT_OPEN,
} InputMode;

typedef struct {
    void (*keypress)(int key);
    void (*update)(void);
} EditorModeOps;

typedef struct {
    EditorStatus status;
    const EditorModeOps *mode_ops[4];
    InputMode input_mode;
    CommandLine cmdline;
    char *home_dir;
    char *user_config_dir;
    char *charset;
    bool child_controls_terminal;
    bool resized;
    int cmdline_x;
    const char *version;
    const char *pkgdatadir;
} EditorState;

extern EditorState editor;

char *editor_file(const char *name);
char get_confirmation(const char *choices, const char *format, ...) FORMAT(2);
void set_input_mode(InputMode mode);
void any_key(void);
void normal_update(void);
void resize(void);
void ui_end(void);
void suspend(void);
void set_signal_handler(int signum, void (*handler)(int));
void main_loop(void);

#endif
