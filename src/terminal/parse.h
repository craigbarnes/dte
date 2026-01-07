#ifndef TERMINAL_PARSE_H
#define TERMINAL_PARSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "key.h"
#include "util/macros.h"

// TODO: Use separate arrays for params and sub-params. We may need to
// store 16 or so params for e.g. DA1 replies, but sub-params are only
// needed for the first 2 (for the Kitty Keyboard Protocol; see related
// comment in parse_csi()).

// Representation of parameters (and other info) extracted from CSI sequences
typedef struct {
    uint32_t params[16][4];
    uint8_t nsub[16]; // Lengths for params arrays (sub-params)
    uint8_t intermediate[4];
    uint8_t nparams;
    uint8_t nr_intermediate;
    uint8_t final_byte;
    bool have_subparams;
    bool unhandled_bytes;
} TermControlParams;

enum {
    // Returned by term_parse_sequence() if buf contains a valid prefix
    // for a known sequence but isn't terminated within the length bound
    // (e.g. because another read(3) call is required to fill the buffer)
    TPARSE_PARTIAL_MATCH = 0,
};

size_t term_parse_sequence(const char *buf, size_t length, KeyCode *k) WARN_UNUSED_RESULT NONNULL_ARGS WRITEONLY(3);
size_t term_parse_csi_params(const char *buf, size_t len, size_t i, TermControlParams *csi) WARN_UNUSED_RESULT NONNULL_ARGS;

#endif
