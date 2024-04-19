#ifndef TERMINAL_PARSE_H
#define TERMINAL_PARSE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include "key.h"
#include "util/macros.h"

typedef struct {
    uint32_t params[4][4];
    uint8_t nsub[4]; // Lengths for params arrays (sub-params)
    uint8_t intermediate[4];
    uint8_t nparams;
    uint8_t nr_intermediate;
    uint8_t final_byte;
    bool have_subparams;
    bool unhandled_bytes;
} ControlParams;

ssize_t term_parse_sequence(const char *buf, size_t length, KeyCode *k) WARN_UNUSED_RESULT;
size_t term_parse_csi_params(const char *buf, size_t len, size_t i, ControlParams *csi) WARN_UNUSED_RESULT;

#endif
