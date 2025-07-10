CONFIG_SECTION static const char builtin_test_data_move_dterc[] = "open\n";

#define cfg(n, t) {.name = n, .text = {.data = t, .length = sizeof(t) - 1}}

CONFIG_SECTION static const BuiltinConfig builtin_configs[] = {
    cfg("test/data/move.dterc", builtin_test_data_move_dterc),
};
