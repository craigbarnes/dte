CONFIG_SECTION static const char builtin_rc[] = "alias exit quit\n";
CONFIG_SECTION static const char builtin_color_reset[] = "hi default\n";

CONFIG_SECTION static const BuiltinConfig builtin_configs[] = {
    CFG("rc", builtin_rc),
    CFG("color/reset", builtin_color_reset),
};
