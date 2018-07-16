#include <langinfo.h>
#include <locale.h>
#include "test.h"
#include "../src/editor.h"
#include "../src/path.h"
#include "../src/encoding.h"
#include "../src/filetype.h"
#include "../src/lookup/xterm-keys.c"

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

static void test_parse_xterm_key_sequence(void)
{
    static const struct xterm_key_test {
        const char *escape_sequence;
        ssize_t expected_length;
        Key expected_key;
    } tests[] = {
        {"\033[Z", 3, MOD_SHIFT | '\t'},
        {"\033[1;2A", 6, MOD_SHIFT | KEY_UP},
        {"\033[1;2", -1, 0},
        {"\033[", -1, 0},
        {"\033O", -1, 0},
        {"\033[\033", 0, 0},
        {"\033[A", 3, KEY_UP},
        {"\033[F", 3, KEY_END},
        {"\033[H", 3, KEY_HOME},
        {"\033[L", 3, KEY_INSERT},
        {"\033[1~", 4, KEY_HOME},
        {"\033[5~", 4, KEY_PAGE_UP},
        {"\033[6~", 4, KEY_PAGE_DOWN},
        {"\033OD", 3, KEY_LEFT},
        {"\033OF", 3, KEY_END},
        {"\033OH", 3, KEY_HOME},
        {"\033OP", 3, KEY_F1},
        {"\033OQ", 3, KEY_F2},
        {"\033OR", 3, KEY_F3},
        {"\033OS", 3, KEY_F4},
        {"\033[15~", 5, KEY_F5},
        {"\033[17~", 5, KEY_F6},
        {"\033[23~", 5, KEY_F11},
        {"\033[24~", 5, KEY_F12},
        {"\033[6;3~", 6, MOD_META | KEY_PAGE_DOWN},
        {"\033[6;5~", 6, MOD_CTRL | KEY_PAGE_DOWN},
        {"\033[6;8~", 6, MOD_SHIFT | MOD_META | MOD_CTRL | KEY_PAGE_DOWN},
    };
    FOR_EACH_I(i, tests) {
        const char *seq = tests[i].escape_sequence;
        Key key;
        ssize_t length = parse_xterm_key_sequence(seq, strlen(seq), &key);
        IEXPECT_EQ(length, tests[i].expected_length, i, "lengths");
        if (length > 0) {
            IEXPECT_EQ(key, tests[i].expected_key, i, "keys");
        }
    }
}

void test_util_ascii(void);
void test_key_to_string(void);

int main(void)
{
    init_editor_state();

    test_relative_filename();
    test_detect_encoding_from_bom();
    test_find_ft_filename();
    test_find_ft_firstline();
    test_parse_xterm_key_sequence();
    test_util_ascii();
    test_key_to_string();

    return failed ? 1 : 0;
}
