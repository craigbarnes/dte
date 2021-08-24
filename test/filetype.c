#include <string.h>
#include "test.h"
#include "filetype.h"

static void test_find_ft_filename(void)
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
        // TODO: {".c~", NULL},
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
        {NULL, NULL},
    };
    const StringView empty_line = STRING_VIEW_INIT;
    FOR_EACH_I(i, tests) {
        const char *ft = find_ft(tests[i].filename, empty_line);
        IEXPECT_STREQ(ft, tests[i].expected_filetype);
    }
}

static void test_find_ft_firstline(void)
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
        {"#!/usr/bin/coffee", "coffeescript"},
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
    FOR_EACH_I(i, tests) {
        const char *ft = find_ft(NULL, strview_from_cstring(tests[i].line));
        IEXPECT_STREQ(ft, tests[i].expected_filetype);
    }
}

static void test_find_ft_dynamic(void)
{
    const char *ft = "test1";
    StringView line = STRING_VIEW_INIT;
    EXPECT_FALSE(is_ft(ft));
    add_filetype(ft, "ext-test", FT_EXTENSION);
    EXPECT_TRUE(is_ft(ft));
    EXPECT_STREQ(find_ft("/tmp/file.ext-test", line), ft);

    ft = "test2";
    add_filetype(ft, "/zdir/__[A-Z]+$", FT_FILENAME);
    EXPECT_STREQ(find_ft("/tmp/zdir/__TESTFILE", line), ft);
    EXPECT_STREQ(find_ft("/tmp/zdir/__testfile", line), NULL);

    ft = "test3";
    add_filetype(ft, "._fiLeName", FT_BASENAME);
    EXPECT_STREQ(find_ft("/tmp/._fiLeName", line), ft);
    EXPECT_STREQ(find_ft("/tmp/._filename", line), NULL);

    ft = "test4";
    line = strview_from_cstring("!!42");
    add_filetype(ft, "^!+42$", FT_CONTENT);
    EXPECT_STREQ(find_ft(NULL, line), ft);

    ft = "test5";
    line = strview_from_cstring("#!/usr/bin/xyzlang4.2");
    add_filetype(ft, "xyzlang", FT_INTERPRETER);
    EXPECT_STREQ(find_ft(NULL, line), ft);
}

static void test_is_ft(void)
{
    EXPECT_TRUE(is_ft("ada"));
    EXPECT_TRUE(is_ft("asm"));
    EXPECT_TRUE(is_ft("awk"));
    EXPECT_TRUE(is_ft("c"));
    EXPECT_TRUE(is_ft("d"));
    EXPECT_TRUE(is_ft("dte"));
    EXPECT_TRUE(is_ft("java"));
    EXPECT_TRUE(is_ft("javascript"));
    EXPECT_TRUE(is_ft("lua"));
    EXPECT_TRUE(is_ft("mail"));
    EXPECT_TRUE(is_ft("make"));
    EXPECT_TRUE(is_ft("pkg-config"));
    EXPECT_TRUE(is_ft("rst"));
    EXPECT_TRUE(is_ft("sh"));
    EXPECT_TRUE(is_ft("yaml"));
    EXPECT_TRUE(is_ft("zig"));

    EXPECT_FALSE(is_ft(""));
    EXPECT_FALSE(is_ft("-"));
    EXPECT_FALSE(is_ft("a"));
    EXPECT_FALSE(is_ft("C"));
    EXPECT_FALSE(is_ft("MAKE"));

    EXPECT_TRUE(is_ft("test1"));
    EXPECT_TRUE(is_ft("test2"));
    EXPECT_TRUE(is_ft("test3"));
    EXPECT_TRUE(is_ft("test4"));
    EXPECT_TRUE(is_ft("test5"));
    EXPECT_FALSE(is_ft("test0"));
    EXPECT_FALSE(is_ft("test"));
}

static const TestEntry tests[] = {
    TEST(test_find_ft_filename),
    TEST(test_find_ft_firstline),
    TEST(test_find_ft_dynamic),
    TEST(test_is_ft),
};

const TestGroup filetype_tests = TEST_GROUP(tests);
