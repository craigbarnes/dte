#include <langinfo.h>
#include <locale.h>
#include <string.h>
#include "test.h"
#include "../src/command.h"
#include "../src/editor.h"
#include "../src/encoding.h"
#include "../src/filetype.h"
#include "../src/lookup/xterm-keys.c"
#include "../src/util/path.h"

void test_util(void);
void test_key_to_string(void);
void init_headless_mode(void);
void test_exec_config(void);

unsigned int failed;

void fail(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    failed += 1;
}

static void test_relative_filename(void)
{
    static const struct rel_test {
        const char *cwd, *path, *result;
    } tests[] = { // NOTE: at most 2 ".." components allowed in relative name
        { "/", "/", "/" },
        { "/", "/file", "file" },
        { "/a/b/c/d", "/a/b/file", "../../file" },
        { "/a/b/c/d/e", "/a/b/file", "/a/b/file" },
        { "/a/foobar", "/a/foo/file", "../foo/file" },
    };
    FOR_EACH_I(i, tests) {
        const struct rel_test *t = &tests[i];
        char *result = relative_filename(t->path, t->cwd);
        EXPECT_STREQ(t->result, result);
        free(result);
    }
}

static void test_detect_encoding_from_bom(void)
{
    static const struct bom_test {
        const char *encoding;
        const unsigned char *text;
        size_t size;
    } tests[] = {
        {"UTF-8", "\xef\xbb\xbfHello", 8},
        {"UTF-32BE", "\x00\x00\xfe\xffHello", 9},
        {"UTF-32LE", "\xff\xfe\x00\x00Hello", 9},
        {"UTF-16BE", "\xfe\xffHello", 7},
        {"UTF-16LE", "\xff\xfeHello", 7},
        {NULL, "\x00\xef\xbb\xbfHello", 9},
        {NULL, "\xef\xbb", 2},
    };
    FOR_EACH_I(i, tests) {
        const struct bom_test *t = &tests[i];
        const char *result = detect_encoding_from_bom(t->text, t->size);
        EXPECT_STREQ(result, t->encoding);
    }
}

static void test_find_ft_filename(void)
{
    static const struct ft_filename_test {
        const char *filename, *expected_filetype;
    } tests[] = {
        {"/usr/local/include/lib.h", "c"},
        {"test.cc~", "c"},
        {"test.c.pacnew", "c"},
        {"test.c.pacnew~", "c"},
        {"test.lua", "lua"},
        {"test.py", "python"},
        {"makefile", "make"},
        {"GNUmakefile", "make"},
        {".file.yml", "yaml"},
        {"/etc/nginx.conf", "nginx"},
        {"file.c.old~", "c"},
        {"file..rb", "ruby"},
        {"file.rb", "ruby"},
        {"/etc/hosts", "config"},
        {"/etc/fstab", "config"},
        {"/boot/grub/menu.lst", "config"},
        {"/etc/krb5.conf", "ini"},
        {"/etc/ld.so.conf", "config"},
        {"/etc/default/grub", "sh"},
        {"/etc/systemd/user.conf", "ini"},
        {"/etc/nginx/mime.types", "nginx"},
        {"", NULL},
        {"/", NULL},
        {"/etc../etc.c.old/c.old", NULL},
    };
    FOR_EACH_I(i, tests) {
        const struct ft_filename_test *t = &tests[i];
        const char *result = find_ft(t->filename, NULL, NULL, 0);
        EXPECT_STREQ(result, t->expected_filetype);
    }
}

static void test_find_ft_firstline(void)
{
    static const struct ft_firstline_test {
        const char *line, *expected_filetype;
    } tests[] = {
        {"<!DOCTYPE html>", "html"},
        {"<!doctype HTML", "html"},
        {"<!doctype htm", NULL},
        {"<?xml version=\"1.0\" encoding=\"UTF-8\"?>", "xml"},
        {"[wrap-file]", "ini"},
        {"[wrap-file", NULL},
        {".TH DTE 1", NULL},
        {"", NULL},
        {" ", NULL},
        {" <?xml", NULL},
        {"\0<?xml", NULL},
    };
    FOR_EACH_I(i, tests) {
        const struct ft_firstline_test *t = &tests[i];
        const char *result = find_ft(NULL, NULL, t->line, strlen(t->line));
        EXPECT_STREQ(result, t->expected_filetype);
    }
}

static void test_find_ft_interpreter(void)
{
    static const struct ft_interpreter_test {
        const char *interpreter, *expected_filetype;
    } tests[] = {
        {"ash", "sh"},
        {"awk", "awk"},
        {"bash", "sh"},
        {"bigloo", "scheme"},
        {"ccl", "lisp"},
        {"chicken", "scheme"},
        {"clisp", "lisp"},
        {"coffee", "coffeescript"},
        {"crystal", "ruby"},
        {"dash", "sh"},
        {"ecl", "lisp"},
        {"gawk", "awk"},
        {"gmake", "make"},
        {"gnuplot", "gnuplot"},
        {"groovy", "groovy"},
        {"gsed", "sed"},
        {"guile", "scheme"},
        {"jruby", "ruby"},
        {"ksh", "sh"},
        {"lisp", "lisp"},
        {"luajit", "lua"},
        {"lua", "lua"},
        {"macruby", "ruby"},
        {"make", "make"},
        {"mawk", "awk"},
        {"mksh", "sh"},
        {"moon", "moonscript"},
        {"nawk", "awk"},
        {"node", "javascript"},
        {"openrc-run", "sh"},
        {"pdksh", "sh"},
        {"perl", "perl"},
        {"php", "php"},
        {"python", "python"},
        {"r6rs", "scheme"},
        {"racket", "scheme"},
        {"rake", "ruby"},
        {"ruby", "ruby"},
        {"runhaskell", "haskell"},
        {"sbcl", "lisp"},
        {"sed", "sed"},
        {"sh", "sh"},
        {"tcc", "c"},
        {"tclsh", "tcl"},
        {"wish", "tcl"},
        {"zsh", "sh"},
    };
    FOR_EACH_I(i, tests) {
        const struct ft_interpreter_test *t = &tests[i];
        const char *result = find_ft(NULL, t->interpreter, NULL, 0);
        EXPECT_STREQ(result, t->expected_filetype);
    }
}

static void test_parse_xterm_key(void)
{
    static const struct xterm_key_test {
        const char *escape_sequence;
        ssize_t expected_length;
        KeyCode expected_key;
    } tests[] = {
        {"\033[Z", 3, MOD_SHIFT | '\t'},
        {"\033[1;2A", 6, MOD_SHIFT | KEY_UP},
        {"\033[1;2B", 6, MOD_SHIFT | KEY_DOWN},
        {"\033[1;2C", 6, MOD_SHIFT | KEY_RIGHT},
        {"\033[1;2D", 6, MOD_SHIFT | KEY_LEFT},
        {"\033[1;2F", 6, MOD_SHIFT | KEY_END},
        {"\033[1;2H", 6, MOD_SHIFT | KEY_HOME},
        {"\033[1;8H", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_HOME},
        {"\033[", -1, 0},
        {"\033]", 0, 0},
        {"\033[1", -1, 0},
        {"\033[9", 0, 0},
        {"\033[1;", -1, 0},
        {"\033[1[", 0, 0},
        {"\033[1;2", -1, 0},
        {"\033[1;8", -1, 0},
        {"\033[1;9", 0, 0},
        {"\033[1;_", 0, 0},
        {"\033[1;8Z", 0, 0},
        {"\033O", -1, 0},
        {"\033[\033", 0, 0},
        {"\033[A", 3, KEY_UP},
        {"\033[B", 3, KEY_DOWN},
        {"\033[C", 3, KEY_RIGHT},
        {"\033[D", 3, KEY_LEFT},
        {"\033[F", 3, KEY_END},
        {"\033[H", 3, KEY_HOME},
        {"\033[L", 3, KEY_INSERT},
        {"\033[1~", 4, KEY_HOME},
        {"\033[2~", 4, KEY_INSERT},
        {"\033[3~", 4, KEY_DELETE},
        {"\033[4~", 4, KEY_END},
        {"\033[5~", 4, KEY_PAGE_UP},
        {"\033[6~", 4, KEY_PAGE_DOWN},
        {"\033O ", 3, ' '},
        {"\033OA", 3, KEY_UP},
        {"\033OB", 3, KEY_DOWN},
        {"\033OC", 3, KEY_RIGHT},
        {"\033OD", 3, KEY_LEFT},
        {"\033OF", 3, KEY_END},
        {"\033OH", 3, KEY_HOME},
        {"\033OI", 3, '\t'},
        {"\033OM", 3, '\r'},
        {"\033OP", 3, KEY_F1},
        {"\033OQ", 3, KEY_F2},
        {"\033OR", 3, KEY_F3},
        {"\033OS", 3, KEY_F4},
        {"\033Oj", 3, '*'},
        {"\033Ok", 3, '+'},
        {"\033Om", 3, '-'},
        {"\033Oo", 3, '/'},
        {"\033[11~", 5, KEY_F1},
        {"\033[12~", 5, KEY_F2},
        {"\033[13~", 5, KEY_F3},
        {"\033[14~", 5, KEY_F4},
        {"\033[15~", 5, KEY_F5},
        {"\033[17~", 5, KEY_F6},
        {"\033[18~", 5, KEY_F7},
        {"\033[19~", 5, KEY_F8},
        {"\033[20~", 5, KEY_F9},
        {"\033[21~", 5, KEY_F10},
        {"\033[23~", 5, KEY_F11},
        {"\033[24~", 5, KEY_F12},
        {"\033[6;3~", 6, MOD_META | KEY_PAGE_DOWN},
        {"\033[6;5~", 6, MOD_CTRL | KEY_PAGE_DOWN},
        {"\033[6;8~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_PAGE_DOWN},
        {"\033[[A", 4, KEY_F1},
        {"\033[[B", 4, KEY_F2},
        {"\033[[C", 4, KEY_F3},
        {"\033[[D", 4, KEY_F4},
        {"\033[[E", 4, KEY_F5},
    };
    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].escape_sequence;
        KeyCode key;
        ssize_t length = parse_xterm_key(seq, strlen(seq), &key);
        IEXPECT_EQ(length, tests[i].expected_length, i, "lengths");
        if (length > 0) {
            IEXPECT_EQ(key, tests[i].expected_key, i, "keys");
        }
    }
}

static void test_parse_xterm_key_combo(void)
{
    static const struct {
        char escape_sequence[8];
        KeyCode key;
    } templates[] = {
        {"\033[1;_A", KEY_UP},
        {"\033[1;_B", KEY_DOWN},
        {"\033[1;_C", KEY_RIGHT},
        {"\033[1;_D", KEY_LEFT},
        {"\033[1;_F", KEY_END},
        {"\033[1;_H", KEY_HOME},
        {"\033[2;_~", KEY_INSERT},
        {"\033[3;_~", KEY_DELETE},
        {"\033[5;_~", KEY_PAGE_UP},
        {"\033[6;_~", KEY_PAGE_DOWN},
        {"\033[1;_P", KEY_F1},
        {"\033[1;_Q", KEY_F2},
        {"\033[1;_R", KEY_F3},
        {"\033[1;_S", KEY_F4},
        {"\033[15;_~", KEY_F5},
        {"\033[17;_~", KEY_F6},
        {"\033[18;_~", KEY_F7},
        {"\033[19;_~", KEY_F8},
        {"\033[20;_~", KEY_F9},
        {"\033[21;_~", KEY_F10},
        {"\033[23;_~", KEY_F11},
        {"\033[24;_~", KEY_F12},
    };

    static const struct {
        char ch;
        KeyCode mask;
    } modifiers[] = {
        {'2', MOD_SHIFT},
        {'3', MOD_META},
        {'4', MOD_SHIFT | MOD_META},
        {'5', MOD_CTRL},
        {'6', MOD_SHIFT | MOD_CTRL},
        {'7', MOD_META | MOD_CTRL},
        {'8', MOD_SHIFT | MOD_META | MOD_CTRL}
    };

    FOR_EACH_I(i, templates) {
        FOR_EACH_I(j, modifiers) {
            char seq[8];
            memcpy(seq, templates[i].escape_sequence, 8);
            BUG_ON(seq[7] != '\0');
            char *underscore = strchr(seq, '_');
            BUG_ON(underscore == NULL);
            *underscore = modifiers[j].ch;
            size_t seq_length = strlen(seq);
            KeyCode key;
            ssize_t parsed_length = parse_xterm_key(seq, seq_length, &key);
            EXPECT_EQ(parsed_length, seq_length);
            EXPECT_EQ(key, modifiers[j].mask | templates[i].key);
        }
    }
}

static int command_cmp(const void *p1, const void *p2)
{
    const Command *c1 = p1, *c2 = p2;
    return strcmp(c1->name, c2->name);
}

// Checks that `commands` array is sorted in binary searchable order
static void test_commands_sort(void)
{
    size_t n = 0;
    while (commands[n].name) {
        n++;
    }
    EXPECT_EQ(n, 85);

    const size_t size = n * sizeof(Command);
    Command *commands_copy = xmalloc(size);
    memcpy(commands_copy, commands, size);
    qsort(commands_copy, n, sizeof(Command), command_cmp);

    for (size_t i = 0; i < n; i++) {
        EXPECT_STREQ(commands[i].name, commands_copy[i].name);
    }

    free(commands_copy);
}

int main(void)
{
    init_editor_state();

    test_relative_filename();
    test_detect_encoding_from_bom();
    test_find_ft_filename();
    test_find_ft_firstline();
    test_find_ft_interpreter();
    test_parse_xterm_key();
    test_parse_xterm_key_combo();
    test_commands_sort();

    test_util();
    test_key_to_string();

    init_headless_mode();
    test_exec_config();

    return failed ? 1 : 0;
}
