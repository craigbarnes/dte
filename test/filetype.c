#include "test.h"
#include "filetype.h"

static void test_is_valid_filetype_name(TestContext *ctx)
{
    EXPECT_TRUE(is_valid_filetype_name("abc-XYZ_128.90"));
    EXPECT_FALSE(is_valid_filetype_name(""));
    EXPECT_FALSE(is_valid_filetype_name("x/y"));
    EXPECT_FALSE(is_valid_filetype_name("/"));
}

static void test_find_ft_filename(TestContext *ctx)
{
    static const struct {
        const char *filename;
        const char *expected_filetype;
    } tests[] = {
        {"/usr/local/include/lib.h", "c"},
        {"test.cc~", "c"},
        {"test.c++", "c"},
        {"test.lua", "lua"},
        {"test.py", "python"},
        {"test.rs", "rust"},
        {"test.C", "c"},
        {"test.H", "c"},
        {"test.re", "c"},
        {"test.S", "asm"},
        {"test.s", "asm"},
        {"test.d", "d"},
        {"test.l", "lex"},
        {"test.m", "objc"},
        {"test.v", "verilog"},
        {"test.y", "yacc"},
        {"test.kt", "kotlin"},
        {"test.kts", "kotlin"},
        {"test.csv", "csv"},
        {"test.tsv", "tsv"},
        {"test.opml", "xml"},
        {"test.gcode", "gcode"},
        {"makefile", "make"},
        {"GNUmakefile", "make"},
        {".file.yml", "yaml"},
        {"/etc/nginx.conf", "nginx"},
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
        {"/etc/pam.d/login", "config"},
        {"/etc/iptables/iptables.rules", "config"},
        {"/etc/iptables/ip6tables.rules", "config"},
        {"/root/.bash_profile", "sh"},
        {".bash_profile", "sh"},
        {".clang-format", "yaml"},
        {".drirc", "xml"},
        {".editorconfig", "ini"},
        {".gitattributes", "config"},
        {".gitconfig", "ini"},
        {".gitmodules", "ini"},
        {".jshintrc", "json"},
        {".zshrc", "sh"},
        {".XCompose", "config"},
        {".tmux.conf", "tmux"},
        {"zshrc", "sh"},
        {"gnus", "lisp"},
        {"file.flatpakref", "ini"},
        {"file.automount", "ini"},
        {"file.nginxconf", "nginx"},
        {"meson_options.txt", "meson"},
        {"Makefile.am", "make"},
        {"Makefile.in", "make"},
        {"Makefile.i_", NULL},
        {"Makefile.a_", NULL},
        {"Makefile._m", NULL},
        {"M_______.am", NULL},
        {".Makefile", NULL},
        {"file.glslf", "glsl"},
        {"file.glslv", "glsl"},
        {"file.gl_lv", NULL},
        {"file.gls_v", NULL},
        {"file.gl__v", NULL},
        {"._", NULL},
        {".1234", NULL},
        {".12345", NULL},
        {".123456", NULL},
        {".doa_", NULL},
        {"", NULL},
        {"/", NULL},
        {"/etc../etc.c.old/c.old", NULL},
        {".c~", NULL},
        {".ini~", NULL},
        {".yml.bak", NULL},
        {"test.c.bak", "c"},
        {"test.c.new", "c"},
        {"test.c.old~", "c"},
        {"test.c.orig", "c"},
        {"test.c.pacnew", "c"},
        {"test.c.pacorig", "c"},
        {"test.c.pacsave", "c"},
        {"test.c.dpkg-dist", "c"},
        {"test.c.dpkg-old", "c"},
        {"test.c.dpkg-backup", "c"},
        {"test.c.dpkg-remove", "c"},
        {"test.c.rpmnew", "c"},
        {"test.c.rpmsave", "c"},
        {".c", NULL},
        {"test.@", NULL},
        {"test.~", NULL},
        {"1", NULL},
        {NULL, NULL},
    };
    const PointerArray arr = PTR_ARRAY_INIT;
    const StringView empty_line = STRING_VIEW_INIT;
    FOR_EACH_I(i, tests) {
        const char *ft = find_ft(&arr, tests[i].filename, empty_line);
        IEXPECT_STREQ(ft, tests[i].expected_filetype);
    }
}

static void test_find_ft_firstline(TestContext *ctx)
{
    static const struct {
        const char *line;
        const char *expected_filetype;
    } tests[] = {
        {"<!DOCTYPE html>", "html"},
        {"<!doctype HTML", "html"},
        {"<!doctype htm", NULL},
        {"<!DOCTYPE fontconfig", "xml"},
        {"<!DOCTYPE colormap", "xml"},
        {"<!DOCTYPE busconfig", "xml"},
        {"<!doctype busconfig", NULL},
        {"<?xml version=\"1.0\" encoding=\"UTF-8\"?>", "xml"},
        {"%YAML 1.1", "yaml"},
        {"[wrap-file]", "ini"},
        {"[wrap-file", NULL},
        {"[section] \t", "ini"},
        {" [section]", NULL},
        {"[section \t", NULL},
        {"[ section]", NULL},
        {"[1]", NULL},
        {"diff --git a/example.txt b/example.txt", "diff"},
        {".TH DTE 1", NULL},
        {"", NULL},
        {" ", NULL},
        {" <?xml", NULL},
        {"\0<?xml", NULL},
        // Hashbangs
        {"#!/usr/bin/ash", "sh"},
        {"#!/usr/bin/awk", "awk"},
        {"#!/usr/bin/bash", "sh"},
        {"#!/usr/bin/bigloo", "scheme"},
        {"#!/usr/bin/ccl", "lisp"},
        {"#!/usr/bin/chicken", "scheme"},
        {"#!/usr/bin/clisp", "lisp"},
        {"#!/usr/bin/coffee", "coffee"},
        {"#!/usr/bin/crystal", "ruby"},
        {"#!/usr/bin/dash", "sh"},
        {"#!/usr/bin/ecl", "lisp"},
        {"#!/usr/bin/gawk", "awk"},
        {"#!/usr/bin/gmake", "make"},
        {"#!/usr/bin/gnuplot", "gnuplot"},
        {"#!/usr/bin/groovy", "groovy"},
        {"#!/usr/bin/gsed", "sed"},
        {"#!/usr/bin/guile", "scheme"},
        {"#!/usr/bin/jruby", "ruby"},
        {"#!/usr/bin/ksh", "sh"},
        {"#!/usr/bin/lisp", "lisp"},
        {"#!/usr/bin/luajit", "lua"},
        {"#!/usr/bin/lua", "lua"},
        {"#!/usr/bin/macruby", "ruby"},
        {"#!/usr/bin/make", "make"},
        {"#!/usr/bin/mawk", "awk"},
        {"#!/usr/bin/mksh", "sh"},
        {"#!/usr/bin/moon", "moonscript"},
        {"#!/usr/bin/nawk", "awk"},
        {"#!/usr/bin/node", "javascript"},
        {"#!/usr/bin/openrc-run", "sh"},
        {"#!/usr/bin/pdksh", "sh"},
        {"#!/usr/bin/perl", "perl"},
        {"#!/usr/bin/php", "php"},
        {"#!/usr/bin/python", "python"},
        {"#!/usr/bin/r6rs", "scheme"},
        {"#!/usr/bin/racket", "scheme"},
        {"#!/usr/bin/rake", "ruby"},
        {"#!/usr/bin/ruby", "ruby"},
        {"#!/usr/bin/runhaskell", "haskell"},
        {"#!/usr/bin/sbcl", "lisp"},
        {"#!/usr/bin/sed", "sed"},
        {"#!/bin/sh", "sh"},
        {"#!/usr/bin/tcc", "c"},
        {"#!/usr/bin/tclsh", "tcl"},
        {"#!/usr/bin/wish", "tcl"},
        {"#!/usr/bin/zsh", "sh"},
        {"#!/usr/bin/lua5.3", "lua"},
        {"#!/usr/bin/env lua", "lua"},
        {"#!/usr/bin/env lua5.3", "lua"},
        {"#!  /usr/bin/env lua5.3", "lua"},
        {"#!  /usr/bin/env  lua5.3", "lua"},
        {"#!/usr/bin/env /usr/bin/lua", "lua"},
        {"#!/lua", "lua"},
        {"#!/usr/bin/_unhaskell", NULL},
        {"#!/usr/bin/runhaskel_", NULL},
        {"#!/usr/bin/unknown", NULL},
        {"#!lua", NULL},
        {"#!/usr/bin/ lua", NULL},
        {"#!/usr/bin/", NULL},
        {"#!/usr/bin/env", NULL},
        {"#!/usr/bin/env ", NULL},
        {"#!/usr/bin/env  ", NULL},
        {"#!/usr/bin/env/ lua", NULL},
        {"#!/lua/", NULL},
    };
    const PointerArray arr = PTR_ARRAY_INIT;
    FOR_EACH_I(i, tests) {
        const char *ft = find_ft(&arr, NULL, strview_from_cstring(tests[i].line));
        IEXPECT_STREQ(ft, tests[i].expected_filetype);
    }
}

static void test_find_ft_dynamic(TestContext *ctx)
{
    PointerArray a = PTR_ARRAY_INIT;
    const char *ft = "test1";
    StringView line = STRING_VIEW_INIT;
    EXPECT_FALSE(is_ft(&a, ft));
    add_filetype(&a, ft, "ext-test", FT_EXTENSION);
    EXPECT_TRUE(is_ft(&a, ft));
    EXPECT_STREQ(find_ft(&a, "/tmp/file.ext-test", line), ft);

    ft = "test2";
    add_filetype(&a, ft, "/zdir/__[A-Z]+$", FT_FILENAME);
    EXPECT_STREQ(find_ft(&a, "/tmp/zdir/__TESTFILE", line), ft);
    EXPECT_STREQ(find_ft(&a, "/tmp/zdir/__testfile", line), NULL);

    ft = "test3";
    add_filetype(&a, ft, "._fiLeName", FT_BASENAME);
    EXPECT_STREQ(find_ft(&a, "/tmp/._fiLeName", line), ft);
    EXPECT_STREQ(find_ft(&a, "/tmp/._filename", line), NULL);

    ft = "test4";
    line = strview_from_cstring("!!42");
    add_filetype(&a, ft, "^!+42$", FT_CONTENT);
    EXPECT_STREQ(find_ft(&a, NULL, line), ft);

    ft = "test5";
    line = strview_from_cstring("#!/usr/bin/xyzlang4.2");
    add_filetype(&a, ft, "xyzlang", FT_INTERPRETER);
    EXPECT_STREQ(find_ft(&a, NULL, line), ft);

    EXPECT_TRUE(is_ft(&a, "test1"));
    EXPECT_TRUE(is_ft(&a, "test2"));
    EXPECT_TRUE(is_ft(&a, "test3"));
    EXPECT_TRUE(is_ft(&a, "test4"));
    EXPECT_TRUE(is_ft(&a, "test5"));
    EXPECT_FALSE(is_ft(&a, "test0"));
    EXPECT_FALSE(is_ft(&a, "test"));

    free_filetypes(&a);
}

static void test_is_ft(TestContext *ctx)
{
    const PointerArray a = PTR_ARRAY_INIT;
    EXPECT_TRUE(is_ft(&a, "ada"));
    EXPECT_TRUE(is_ft(&a, "asm"));
    EXPECT_TRUE(is_ft(&a, "awk"));
    EXPECT_TRUE(is_ft(&a, "c"));
    EXPECT_TRUE(is_ft(&a, "d"));
    EXPECT_TRUE(is_ft(&a, "dte"));
    EXPECT_TRUE(is_ft(&a, "java"));
    EXPECT_TRUE(is_ft(&a, "javascript"));
    EXPECT_TRUE(is_ft(&a, "lua"));
    EXPECT_TRUE(is_ft(&a, "mail"));
    EXPECT_TRUE(is_ft(&a, "make"));
    EXPECT_TRUE(is_ft(&a, "pkg-config"));
    EXPECT_TRUE(is_ft(&a, "rst"));
    EXPECT_TRUE(is_ft(&a, "sh"));
    EXPECT_TRUE(is_ft(&a, "yaml"));
    EXPECT_TRUE(is_ft(&a, "zig"));

    EXPECT_FALSE(is_ft(&a, ""));
    EXPECT_FALSE(is_ft(&a, "-"));
    EXPECT_FALSE(is_ft(&a, "a"));
    EXPECT_FALSE(is_ft(&a, "C"));
    EXPECT_FALSE(is_ft(&a, "MAKE"));
}

static const TestEntry tests[] = {
    TEST(test_is_valid_filetype_name),
    TEST(test_find_ft_filename),
    TEST(test_find_ft_firstline),
    TEST(test_find_ft_dynamic),
    TEST(test_is_ft),
};

const TestGroup filetype_tests = TEST_GROUP(tests);
