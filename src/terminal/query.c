#include "query.h"
#include "cursor.h"
#include "terminal.h"
#include "util/log.h"
#include "util/string-view.h"
#include "util/strtonum.h"

// https://vt100.net/docs/vt510-rm/DECRPM
typedef enum {
    DECRPM_NOT_RECOGNIZED = 0,
    DECRPM_SET = 1,
    DECRPM_RESET = 2,
    DECRPM_PERMANENTLY_SET = 3,
    DECRPM_PERMANENTLY_RESET = 4,
} TermPrivateModeStatus;

static const char *decrpm_status_to_str(TermPrivateModeStatus val)
{
    switch (val) {
    case DECRPM_NOT_RECOGNIZED: return "not recognized";
    case DECRPM_SET: return "set";
    case DECRPM_RESET: return "reset";
    case DECRPM_PERMANENTLY_SET: return "permanently set";
    case DECRPM_PERMANENTLY_RESET: return "permanently reset";
    }
    return "INVALID";
}

static bool decrpm_is_set_or_reset(TermPrivateModeStatus status)
{
    return status == DECRPM_SET || status == DECRPM_RESET;
}

KeyCode parse_csi_query_reply(const ControlParams *csi, uint8_t prefix)
{
    if (unlikely(csi->have_subparams || csi->nr_intermediate > 1)) {
        goto ignore;
    }

    // NOTE: The conditions below must check ALL of these values, in
    // addition to the prefix byte
    uint8_t final = csi->final_byte;
    uint8_t nparams = csi->nparams;
    uint8_t intermediate = csi->nr_intermediate ? csi->intermediate[0] : 0;

    if (prefix == '?' && final == 'y' && intermediate == '$' && nparams == 2) {
        // DECRPM reply to DECRQM query (CSI ? Ps; Pm $ y)
        unsigned int mode = csi->params[0][0];
        unsigned int status = csi->params[1][0];
        const char *desc = decrpm_status_to_str(status);
        LOG_DEBUG("DECRPM %u reply: %u (%s)", mode, status, desc);
        if (mode == 2026 && decrpm_is_set_or_reset(status)) {
            return KEYCODE_QUERY_REPLY_BIT | TFLAG_SYNC;
        }
        return KEY_IGNORE;
    }

    if (prefix == '?' && final == 'u' && intermediate == 0 && nparams == 1) {
        // Kitty keyboard protocol flags (CSI ? flags u)
        unsigned int flags = csi->params[0][0];
        LOG_DEBUG("query reply for kittykbd flags: 0x%x", flags);
        // Interpret reply with any flags to mean "supported"
        return KEYCODE_QUERY_REPLY_BIT | TFLAG_KITTY_KEYBOARD;
    }

    if (prefix == '>' && final == 'm' && intermediate == 0 && nparams >= 1) {
        unsigned int code = csi->params[0][0];
        if (code == 4 && nparams <= 2) {
            // XTMODKEYS 4 reply to XTQMODKEYS 4 query (CSI > 4 ; Pv m)
            unsigned int val = (nparams == 1) ? 0 : csi->params[1][0];
            LOG_DEBUG("XTMODKEYS 4 reply: modifyOtherKeys=%u", val);
            return KEY_IGNORE;
        }
        LOG_DEBUG("XTMODKEYS %u reply with %u params", code, nparams);
        return KEY_IGNORE;
    }

ignore:
    LOG_DEBUG("unhandled CSI with '%c' parameter prefix", prefix);
    return KEY_IGNORE;
}

KeyCode parse_dcs_query_reply(const char *data, size_t len, bool truncated)
{
    const char *note = "";
    if (unlikely(len == 0 || truncated)) {
        note = truncated ? " (truncated)" : " (empty)";
        goto unhandled;
    }

    StringView seq = string_view(data, len);
    if (strview_has_prefix(&seq, "1+r")) {
        strview_remove_prefix(&seq, 3);
        if (strview_equal_cstring(&seq, "626365")) { // "bce"
            return KEYCODE_QUERY_REPLY_BIT | TFLAG_BACK_COLOR_ERASE;
        }
        if (strview_equal_cstring_icase(&seq, "74736C=1B5D323B")) { // "tsl=\e]2;"
            return KEYCODE_QUERY_REPLY_BIT | TFLAG_SET_WINDOW_TITLE;
        }
        if (
            strview_has_prefix(&seq, "726570=") // "rep="
            && strview_has_suffix(&seq, "62") // "b"
            && (seq.length & 1) // Odd length (2x even hex strings + "=")
        ) {
            return KEYCODE_QUERY_REPLY_BIT | TFLAG_ECMA48_REPEAT;
        }
        LOG_DEBUG("unhandled XTGETTCAP reply: %.*s", (int)len, data);
        return KEY_IGNORE;
    }

    if (strview_has_prefix(&seq, "1$r")) {
        strview_remove_prefix(&seq, 3);
        if (strview_has_suffix(&seq, " q")) {
            size_t n = seq.length - 2;
            unsigned int x;
            if (n >= 1 && n == buf_parse_uint(seq.data, n, &x)) {
                const char *str = (x <= CURSOR_STEADY_BAR) ? cursor_type_to_str(x) : "??";
                LOG_DEBUG("DECRQSS DECSCUSR (cursor style) reply: %u (%s)", x, str);
                return KEY_IGNORE;
            }
        }
        // TODO: Detect RGB and italic support from DECRQSS "1$r0;3;38:2::60:70:80m" reply
        // TODO: To facilitate the above, convert Terminal::color_type to TermFeatureFlags bits
        LOG_DEBUG("unhandled DECRQSS reply: %.*s", (int)len, data);
        return KEY_IGNORE;
    }

    if (strview_has_prefix(&seq, "0+r")) {
        note = " (XTGETTCAP; 'invalid request')";
        goto unhandled;
    }

    if (strview_equal_cstring(&seq, "0$r")) {
        note = " (DECRQSS; 'invalid request')";
        goto unhandled;
    }

unhandled:
    LOG_DEBUG("unhandled DCS string%s: %.*s", note, (int)len, data);
    return KEY_IGNORE;
}
