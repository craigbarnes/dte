#include "test.h"
#include "../src/cmdline.h"
#include "../src/completion.h"
#include "../src/editor.h"

#define EXPECT_STRING_EQ(s_, cstr_) \
    string_ensure_null_terminated(&(s_)); \
    EXPECT_STREQ((s_).buffer, (cstr_))

static void test_cmdline_handle_key(void)
{
    CommandLine *c = &editor.cmdline;
    PointerArray *h = &editor.command_history;

    int ret = cmdline_handle_key(c, h, 'a');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    ret = cmdline_handle_key(c, h, 0x1F999);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 5);
    EXPECT_STRING_EQ(c->buf, "a\xF0\x9F\xA6\x99");

    // Delete at end-of-line should do nothing
    ret = cmdline_handle_key(c, h, KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 5);
    EXPECT_EQ(c->buf.len, 5);

    ret = cmdline_handle_key(c, h, CTRL('H'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);
    EXPECT_STRING_EQ(c->buf, "a");

    ret = cmdline_handle_key(c, h, CTRL('?'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "");

    cmdline_set_text(c, "word1 word2 word3 word4");
    ret = cmdline_handle_key(c, h, KEY_END);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 23);

    ret = cmdline_handle_key(c, h, MOD_META | MOD_CTRL | 'H');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 18);
    EXPECT_STRING_EQ(c->buf, "word1 word2 word3 ");

    ret = cmdline_handle_key(c, h, MOD_META | MOD_CTRL | '?');
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 12);
    EXPECT_STRING_EQ(c->buf, "word1 word2 ");

    ret = cmdline_handle_key(c, h, CTRL(KEY_LEFT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 6);

    ret = cmdline_handle_key(c, h, CTRL(KEY_RIGHT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 12);

    ret = cmdline_handle_key(c, h, KEY_HOME);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    ret = cmdline_handle_key(c, h, MOD_META | KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "word2 ");

    ret = cmdline_handle_key(c, h, KEY_RIGHT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);

    ret = cmdline_handle_key(c, h, CTRL('U'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "ord2 ");

    ret = cmdline_handle_key(c, h, KEY_DELETE);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_STRING_EQ(c->buf, "rd2 ");

    ret = cmdline_handle_key(c, h, CTRL('K'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);

    // Left arrow at beginning-of-line should do nothing
    ret = cmdline_handle_key(c, h, KEY_LEFT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    cmdline_set_text(c, "...");
    EXPECT_EQ(c->pos, 3);
    EXPECT_EQ(c->buf.len, 3);

    ret = cmdline_handle_key(c, h, CTRL('A'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 3);

    ret = cmdline_handle_key(c, h, KEY_RIGHT);
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 1);

    ret = cmdline_handle_key(c, h, CTRL(KEY_LEFT));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 0);

    ret = cmdline_handle_key(c, h, CTRL('E'));
    EXPECT_EQ(ret, CMDLINE_KEY_HANDLED);
    EXPECT_EQ(c->pos, 3);

    ret = cmdline_handle_key(c, h, CTRL('G'));
    EXPECT_EQ(ret, CMDLINE_CANCEL);
    EXPECT_EQ(c->pos, 0);
    EXPECT_EQ(c->buf.len, 0);
}

void test_complete_command(void)
{
    complete_command();
    EXPECT_STRING_EQ(editor.cmdline.buf, "alias");
    reset_completion();

    cmdline_set_text(&editor.cmdline, "wrap");
    complete_command();
    EXPECT_STRING_EQ(editor.cmdline.buf, "wrap-paragraph ");
    reset_completion();
}

void test_cmdline(void)
{
    test_cmdline_handle_key();
    test_complete_command();
}
