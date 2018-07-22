#include "test.h"
#include "../src/key.h"

void test_key_to_string(void)
{
    static const struct key_to_string_test {
        const char *str;
        KeyCode key;
    } tests[] = {
        {"a", 'a'},
        {"Z", 'Z'},
        {"0", '0'},
        {"{", '{'},
        {"space", ' '},
        {"enter", KEY_ENTER},
        {"tab", '\t'},
        {"insert", KEY_INSERT},
        {"delete", KEY_DELETE},
        {"home", KEY_HOME},
        {"end", KEY_END},
        {"pgup", KEY_PAGE_UP},
        {"pgdown", KEY_PAGE_DOWN},
        {"left", KEY_LEFT},
        {"right", KEY_RIGHT},
        {"up", KEY_UP},
        {"down", KEY_DOWN},
        {"C-A", MOD_CTRL | 'A'},
        {"M-S-{", MOD_META | MOD_SHIFT | '{'},
        {"C-S-A", MOD_CTRL | MOD_SHIFT | 'A'},
        {"F1", KEY_F1},
        {"F12", KEY_F12},
        {"M-enter", MOD_META | KEY_ENTER},
        {"M-space", MOD_META | ' '},
        {"S-tab", MOD_SHIFT | '\t'},
        {"C-M-S-F12", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_F12},
        {"C-M-S-up", MOD_CTRL | MOD_META | MOD_SHIFT | KEY_UP},
        {"C-M-delete", MOD_CTRL | MOD_META | KEY_DELETE},
        {"C-home", MOD_CTRL | KEY_HOME},
    };
    FOR_EACH_I(i, tests) {
        char *str = key_to_string(tests[i].key);
        EXPECT_STREQ(str, tests[i].str);
        free(str);
    }
}
