#include "cmdline.h"
#include "editor.h"
#include "error.h"
#include "mode.h"
#include "screen.h"
#include "spawn.h"
#include "terminal/output.h"
#include "terminal/terminal.h"
#include "util/ascii.h"
#include "util/ptr-array.h"
#include "util/utf8.h"
#include "util/unicode.h"
#include "util/xmalloc.h"
#include "window.h"

static struct {
    PointerArray files;
    char *all_files;
    size_t size;
    size_t selected;
    size_t scroll;
    char line_ending;
} menu;

static void menu_clear(void)
{
    free(menu.all_files);
    menu.all_files = NULL;
    menu.size = 0;
    menu.files.count = 0;
    menu.selected = 0;
    menu.scroll = 0;
    menu.line_ending = '\n';
}

static void menu_load(char **argv, bool null_delimited)
{
    String output = STRING_INIT;
    if (!spawn_source(argv, &output)) {
        set_input_mode(INPUT_NORMAL);
        return;
    }
    menu.all_files = string_steal(&output, &menu.size);
    menu.line_ending = null_delimited ? '\0' : '\n';
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
    size_t i = 0;
    while (str[i]) {
        while (ascii_isspace(str[i])) {
            i++;
        }
        if (!str[i]) {
            break;
        }
        const size_t s = i++;
        while (str[i] && !ascii_isspace(str[i])) {
            i++;
        }
        ptr_array_append(words, xstrslice(str, s, i));
    }
}

static bool words_match(const char *name, const PointerArray *words)
{
    for (size_t i = 0, n = words->count; i < n; i++) {
        if (!strstr(name, words->ptrs[i])) {
            return false;
        }
    }
    return true;
}

static bool words_match_icase(const char *name, const PointerArray *words)
{
    for (size_t i = 0, n = words->count; i < n; i++) {
        if (u_str_index(name, words->ptrs[i]) < 0) {
            return false;
        }
    }
    return true;
}

static const char *selected_file(void)
{
    if (menu.files.count == 0) {
        return NULL;
    }
    return menu.files.ptrs[menu.selected];
}

static void menu_filter(void)
{
    const char *str = string_borrow_cstring(&editor.cmdline.buf);
    const char line_ending = menu.line_ending;
    char *ptr = menu.all_files;
    char *end = menu.all_files + menu.size;
    bool (*match)(const char*, const PointerArray*) = words_match_icase;
    PointerArray words = PTR_ARRAY_INIT;

    // NOTE: words_match_icase() requires str to be lowercase
    if (contains_upper(str)) {
        match = words_match;
    }
    split(&words, str);

    menu.files.count = 0;
    while (ptr < end) {
        char *eol = memchr(ptr, line_ending, end - ptr);
        if (eol == NULL) {
            break;
        }
        *eol = '\0';
        if (match(ptr, &words)) {
            ptr_array_append(&menu.files, ptr);
        }
        ptr = eol + 1;
    }
    ptr_array_free(&words);
    menu.selected = 0;
    menu.scroll = 0;
}

static void up(size_t count)
{
    if (count >= menu.selected) {
        menu.selected = 0;
    } else {
        menu.selected -= count;
    }
}

static void down(size_t count)
{
    if (menu.files.count > 1) {
        menu.selected += count;
        if (menu.selected >= menu.files.count) {
            menu.selected = menu.files.count - 1;
        }
    }
}

static void open_selected(void)
{
    const char *sel = selected_file();
    if (sel != NULL) {
        window_open_file(window, sel, NULL);
    }
}

void menu_reload(char **argv, bool null_delimited)
{
    menu_clear();
    menu_load(argv, null_delimited);
    menu_filter();
}

static size_t terminal_page_height(void)
{
    if (terminal.height >= 6) {
        return terminal.height - 2;
    } else {
        return 1;
    }
}

static void menu_keypress(KeyCode key)
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
        if (menu.files.count > 1) {
            menu.selected = menu.files.count - 1;
        }
        break;
    case MOD_META | 't':
        menu.selected = 0;
        break;
    case KEY_UP:
        up(1);
        break;
    case KEY_DOWN:
        down(1);
        break;
    case KEY_PAGE_UP:
        up(terminal_page_height());
        break;
    case KEY_PAGE_DOWN:
        down(terminal_page_height());
        break;
    case '\t':
        if (menu.selected + 1 >= menu.files.count) {
            menu.selected = 0;
        } else {
            down(1);
        }
        break;
    default:
        switch (cmdline_handle_key(&editor.cmdline, NULL, key)) {
        case CMDLINE_UNKNOWN_KEY:
            break;
        case CMDLINE_KEY_HANDLED:
            menu_filter();
            break;
        case CMDLINE_CANCEL:
            set_input_mode(INPUT_NORMAL);
            break;
        }
    }
    mark_everything_changed();
}

static void menu_update_screen(void)
{
    int x = 0;
    int y = 0;
    int w = terminal.width;
    int h = terminal.height - 1;
    int max_y = menu.scroll + h - 1;
    int i = 0;

    if (h >= menu.files.count) {
        menu.scroll = 0;
    }
    if (menu.scroll > menu.selected) {
        menu.scroll = menu.selected;
    }
    if (menu.selected > max_y) {
        menu.scroll += menu.selected - max_y;
    }

    term_output_reset(x, w, 0);
    terminal.move_cursor(0, 0);
    editor.cmdline_x = print_command('/');
    term_clear_eol();
    y++;

    for (; i < h; i++) {
        int file_idx = menu.scroll + i;
        char *file;
        TermColor color;

        if (file_idx >= menu.files.count) {
            break;
        }

        file = menu.files.ptrs[file_idx];
        obuf.x = 0;
        terminal.move_cursor(x, y + i);

        color = *builtin_colors[BC_DEFAULT];
        if (file_idx == menu.selected) {
            mask_color(&color, builtin_colors[BC_SELECTION]);
        }
        terminal.set_color(&color);
        term_add_str(file);
        term_clear_eol();
    }
    set_builtin_color(BC_DEFAULT);
    for (; i < h; i++) {
        obuf.x = 0;
        terminal.move_cursor(x, y + i);
        term_clear_eol();
    }
}

static void menu_update(void)
{
    term_hide_cursor();
    update_term_title(window->view->buffer);
    menu_update_screen();
    terminal.move_cursor(editor.cmdline_x, 0);
    term_show_cursor();
    term_output_flush();
}

const EditorModeOps menu_ops = {
    .keypress = menu_keypress,
    .update = menu_update,
};
