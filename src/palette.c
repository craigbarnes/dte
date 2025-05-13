#include <stdbool.h>
#include <stdio.h>
#include "palette.h"

static void seq (
    const char *sgr_prefix,
    unsigned int seq_start,
    unsigned int seq_end,
    int width, // Space-padded width of printed number
    const char *suffix,
    bool sgr_starts_at_zero
) {
    for (unsigned int i = seq_start; i <= seq_end; i++) {
        unsigned int sgr = sgr_starts_at_zero ? i - seq_start : i;
        fprintf(stdout, "\033[%s%um %*u%s", sgr_prefix, sgr, width, i, suffix);
    }
}

static void str(const char *s)
{
    fputs(s, stdout);
}

// Print a visual representation of the XTerm style color palette to
// stdout. No effort is made to limit the emitted SGR sequences to the
// set supported by the terminal, or even to detect whether stdout is
// in fact a terminal.
ExitCode print_256_color_palette(void)
{
    // Row 1: basic ECMA-48 palette colors
    str("\033[0m\n   ");
    seq("4", 0, 6, 2, " ", false);
    str("\033[30;47m  7 \033[0m");
    seq("3", 0, 7, 2, " ", false);
    str("\033[0m default \033[0m\n   ");

    // Row 2: aixterm-style "bright" palette colors
    seq("10", 8, 14, 2, " ", true);
    str("\033[30;107m 15 \033[0m");
    seq("9", 8, 15, 2, " ", true);
    str("\033[0;2m     dim\033[0m\n\n");

    // First half of color cube (light foreground)
    str("\033[38;5;253m");
    for (unsigned int i = 0; i < 6; i++) {
        unsigned int start = 16 + (i * 36); // Sequence: 16 52 88 124 160 196
        str("   ");
        seq("48;5;", start, start + 17, 3, "", false);
        str("\033[49m\n");
    }

    // Second half of color cube (dark foreground)
    str("\033[38;5;235m");
    for (unsigned int i = 0; i < 6; i++) {
        unsigned int start = 34 + (i * 36); // Sequence: 34 70 106 142 178 214
        str("   ");
        seq("48;5;", start, start + 17, 3, "", false);
        str("\033[49m\n");
    }

    // First row of grayscales (light foreground)
    str("\033[0;38;5;253m\n   ");
    seq("48;5;", 232, 243, 4, " ", false);

    // Second row of grayscales (dark foreground)
    str("\033[0;38;5;235m\n   ");
    seq("48;5;", 244, 255, 4, " ", false);

    str("\033[0m\n\n");
    return EC_OK;
}
