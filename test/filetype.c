#include <string.h>
#include "test.h"
#include "../src/filetype.h"

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

void test_filetype(void)
{
    test_find_ft_filename();
    test_find_ft_firstline();
    test_find_ft_interpreter();
}
