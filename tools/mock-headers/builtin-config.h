CONFIG_SECTION static const char builtin_rc[] = "alias exit quit\n";
CONFIG_SECTION static const char builtin_color_reset[] = "hi default\n";

#define cfg(n, t) {.name = n, .text = {.data = t, .length = sizeof(t) - 1}}

CONFIG_SECTION static const BuiltinConfig builtin_configs[] = {
    cfg("rc", builtin_rc),
    cfg("color/reset", builtin_color_reset),
};
