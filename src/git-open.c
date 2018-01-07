#include "git-open.h"
#include "spawn.h"
#include "window.h"
#include "view.h"
#include "term.h"
#include "cmdline.h"
#include "editor.h"
#include "obuf.h"
#include "screen.h"
#include "uchar.h"
#include "error.h"

struct git_open git_open;

static void git_open_clear(void)
{
    free(git_open.all_files);
    git_open.all_files = NULL;
    git_open.size = 0;
    git_open.files.count = 0;
    git_open.selected = 0;
    git_open.scroll = 0;
}

static char *cdup(void)
{
    static const char *const cmd[] = {"git", "rev-parse", "--show-cdup", NULL};
    FilterData data;

    data.in = NULL;
    data.in_len = 0;
    if (spawn_filter((char **)cmd, &data)) {
        return NULL;
    }

    const size_t len = data.out_len;
    if (len > 1 && data.out[len - 1] == '\n') {
        data.out[len - 1] = 0;
        return data.out;
    }
    free(data.out);
    return NULL;
}

static void git_open_load(void)
{
    static const char *cmd[] = {"git", "ls-files", "-z", NULL, NULL};
    FilterData data;
    int status = 0;
    char *dir = cdup();

    cmd[3] = dir;

    data.in = NULL;
    data.in_len = 0;
    if ((status = spawn_filter((char **)cmd, &data)) == 0) {
        git_open.all_files = data.out;
        git_open.size = data.out_len;
    } else {
        set_input_mode(INPUT_NORMAL);
        error_msg("git-open: 'git ls-files' command returned %d", status);
    }
    free(dir);
}

static bool contains_upper(const char *str)
{
    size_t i = 0;
    while (str[i]) {
        if (u_is_upper(u_str_get_char(str, &i))) {
            return true;
        }
    }
    return false;
}

static void split(PointerArray *words, const char *str)
{
    size_t s, i = 0;
    while (str[i]) {
        while (ascii_isspace(str[i])) {
            i++;
        }
        if (!str[i]) {
            break;
        }
        s = i++;
        while (str[i] && !ascii_isspace(str[i])) {
            i++;
        }
        ptr_array_add(words, xstrslice(str, s, i));
    }
}

static bool words_match(const char *name, PointerArray *words)
{
    for (size_t i = 0; i < words->count; i++) {
        if (!strstr(name, words->ptrs[i])) {
            return false;
        }
    }
    return true;
}

static bool words_match_icase(const char *name, PointerArray *words)
{
    for (size_t i = 0; i < words->count; i++) {
        if (u_str_index(name, words->ptrs[i]) < 0) {
            return false;
        }
    }
    return true;
}

static const char *selected_file(void)
{
    if (git_open.files.count == 0) {
        return NULL;
    }
    return git_open.files.ptrs[git_open.selected];
}

static void git_open_filter(void)
{
    char *str = string_cstring(&editor.cmdline.buf);
    char *ptr = git_open.all_files;
    char *end = git_open.all_files + git_open.size;
    bool (*match)(const char *, PointerArray *) = words_match_icase;
    PointerArray words = PTR_ARRAY_INIT;

    // NOTE: words_match_icase() requires str to be lowercase
    if (contains_upper(str)) {
        match = words_match;
    }
    split(&words, str);
    free(str);

    git_open.files.count = 0;
    while (ptr < end) {
        char *zero = memchr(ptr, 0, end - ptr);
        if (zero == NULL) {
            break;
        }
        if (match(ptr, &words)) {
            ptr_array_add(&git_open.files, ptr);
        }
        ptr = zero + 1;
    }
    ptr_array_free(&words);
    git_open.selected = 0;
    git_open.scroll = 0;
}

static void up(int count)
{
    git_open.selected -= count;
    if (git_open.selected < 0) {
        git_open.selected = 0;
    }
}

static void down(int count)
{
    git_open.selected += count;
    if (git_open.selected >= git_open.files.count) {
        git_open.selected = git_open.files.count - 1;
    }
}

static void open_selected(void)
{
    const char *sel = selected_file();
    if (sel != NULL) {
        window_open_file(window, sel, NULL);
    }
}

void git_open_reload(void)
{
    git_open_clear();
    git_open_load();
    git_open_filter();
}

void git_open_keypress(Key key)
{
    switch (key) {
    case KEY_ENTER:
        open_selected();
        cmdline_clear(&editor.cmdline);
        set_input_mode(INPUT_NORMAL);
        break;
    case CTRL('O'):
        open_selected();
        down(1);
        break;
    case MOD_META | 'e':
        if (git_open.files.count > 0) {
            git_open.selected = git_open.files.count - 1;
        }
        break;
    case MOD_META | 't':
        git_open.selected = 0;
        break;
    case KEY_UP:
        up(1);
        break;
    case KEY_DOWN:
        down(1);
        break;
    case KEY_PAGE_UP:
        up(terminal.height - 2);
        break;
    case KEY_PAGE_DOWN:
        down(terminal.height - 2);
        break;
    default:
        switch (cmdline_handle_key(&editor.cmdline, NULL, key)) {
        case CMDLINE_UNKNOWN_KEY:
            break;
        case CMDLINE_KEY_HANDLED:
            git_open_filter();
            break;
        case CMDLINE_CANCEL:
            set_input_mode(INPUT_NORMAL);
            break;
        }
    }
    mark_everything_changed();
}

static void git_open_update(void)
{
    buf_hide_cursor();
    update_term_title(window->view->buffer);
    update_git_open();
    buf_move_cursor(editor.cmdline_x, 0);
    buf_show_cursor();
    buf_flush();
}

const EditorModeOps git_open_ops = {
    .keypress = git_open_keypress,
    .update = git_open_update,
};
