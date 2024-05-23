#include "query.h"
#include "cursor.h"
#include "terminal.h"
#include "util/log.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/strtonum.h"
#include "util/utf8.h"

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

static KeyCode tflag(TermFeatureFlags flags)
{
    BUG_ON(flags & KEYCODE_QUERY_REPLY_BIT);
    return KEYCODE_QUERY_REPLY_BIT | flags;
}

static bool csi_has_param(const TermControlParams *csi, uint32_t param, size_t start)
{
    for (size_t i = start, n = csi->nparams; i < n; i++) {
        if (csi->params[i][0] == param) {
            return true;
        }
    }
    return false;
}

KeyCode parse_csi_query_reply(const TermControlParams *csi, uint8_t prefix)
{
    // NOTE: The main conditions below must check ALL of these values, in
    // addition to the prefix byte
    uint8_t intermediate = csi->nr_intermediate ? csi->intermediate[0] : 0;
    uint8_t final = csi->final_byte;
    unsigned int nparams = csi->nparams;

    if (unlikely(csi->have_subparams || csi->nr_intermediate > 1)) {
        goto ignore;
    }

    // https://vt100.net/docs/vt510-rm/DA1.html
    // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
    if (prefix == '?' && final == 'c' && intermediate == 0 && nparams >= 1) {
        unsigned int code = csi->params[0][0];
        if (code >= 61 && code <= 65) {
            unsigned int lvl = code - 60;
            LOG_DEBUG("DA1 reply with P=%u (device level %u)", code, lvl);
            TermFeatureFlags c8 = csi_has_param(csi, 22, 1) ? TFLAG_8_COLOR : 0;
            return tflag(TFLAG_QUERY | c8);
        }
        bool vt100 = (code >= 1 && code <= 12);
        const char *desc = vt100 ? "VT100 series" : "unknown";
        LOG_DEBUG("DA1 reply with P=%u (%s)", code, desc);
        return KEY_IGNORE;
    }

    if (prefix == '?' && final == 'y' && intermediate == '$' && nparams == 2) {
        // DECRPM reply to DECRQM query (CSI ? Ps; Pm $ y)
        unsigned int mode = csi->params[0][0];
        unsigned int status = csi->params[1][0];
        const char *desc = decrpm_status_to_str(status);
        LOG_DEBUG("DECRPM %u reply: %u (%s)", mode, status, desc);
        if (mode == 2026 && decrpm_is_set_or_reset(status)) {
            return tflag(TFLAG_SYNC);
        }
        return KEY_IGNORE;
    }

    if (prefix == '?' && final == 'u' && intermediate == 0 && nparams == 1) {
        // Kitty keyboard protocol flags (CSI ? flags u)
        unsigned int flags = csi->params[0][0];
        LOG_DEBUG("query reply for kittykbd flags: 0x%x", flags);
        // Interpret reply with any flags to mean "supported"
        return tflag(TFLAG_KITTY_KEYBOARD);
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
    // TODO: Also log intermediate and nparams
    LOG_DEBUG("unhandled CSI with '%c' param prefix and final byte '%c'", prefix, (char)final);
    return KEY_IGNORE;
}

static StringView hex_decode_str(StringView input, char *outbuf, size_t bufsize)
{
    StringView empty = STRING_VIEW_INIT;
    size_t n = input.length;
    if (n == 0 || n & 1 || n / 2 > bufsize) {
        return empty;
    }

    for (size_t i = 0, j = 0; i < n; ) {
        unsigned int a = hex_decode(input.data[i++]);
        unsigned int b = hex_decode(input.data[i++]);
        if (unlikely((a | b) > 0xF)) {
            return empty;
        }
        outbuf[j++] = (unsigned char)((a << 4) | b);
    }

    return string_view(outbuf, n / 2);
}

// This is similar to u_make_printable_mem(), but replacing C0 control
// characters with Unicode "control picture" symbols, instead of using
// the (potentially ambiguous) caret notation
static size_t make_printable_ctlseq(const char *src, size_t src_len, char *dest, size_t destsize)
{
    BUG_ON(destsize < 16);
    size_t len = 0;
    for (size_t i = 0; i < src_len && len < destsize - 5; ) {
        CodePoint u = u_get_char(src, src_len, &i);
        u = (u < 0x20) ? u + 0x2400 : (u == 0x7F ? 0x2421 : u);
        len += u_set_char(dest + len, u);
    }
    dest[len] = '\0';
    return len;
}

static KeyCode parse_xtgettcap_reply(const char *data, size_t len)
{
    size_t pos = 3;
    StringView empty = STRING_VIEW_INIT;
    StringView cap_hex = (pos < len) ? get_delim(data, &pos, len, '=') : empty;
    StringView val_hex = (pos < len) ? string_view(data + pos, len - pos) : empty;

    char cbuf[8], vbuf[64];
    StringView cap = hex_decode_str(cap_hex, cbuf, sizeof(cbuf));
    StringView val = hex_decode_str(val_hex, vbuf, sizeof(vbuf));

    if (data[0] == '1' && cap.length >= 2) {
        if (strview_equal_cstring(&cap, "bce") && val.length == 0) {
            return tflag(TFLAG_BACK_COLOR_ERASE);
        }
        if (strview_equal_cstring(&cap, "tsl") && strview_equal_cstring(&val, "\033]2;")) {
            return tflag(TFLAG_SET_WINDOW_TITLE);
        }
        if (strview_equal_cstring(&cap, "rep") && strview_has_suffix(&val, "b")) {
            return tflag(TFLAG_ECMA48_REPEAT);
        }
        if (strview_equal_cstring(&cap, "Ms") && val.length >= 6) {
            // All 65 entries with this cap in the ncurses terminfo database
            // use OSC 52, with only slight differences (BEL vs. ST), so
            // there's really no reason to check the value
            return tflag(TFLAG_OSC52_COPY);
        }
    }

    char ebuf[8 + (4 * sizeof(cbuf)) + (4 * sizeof(vbuf))];
    size_t i = 0;
    if (cap.length) {
        ebuf[i++] = ' ';
        ebuf[i++] = '(';
        i += make_printable_ctlseq(cap.data, cap.length, ebuf + i, sizeof(ebuf) - i);
        if (val.length) {
            ebuf[i++] = '=';
            i += make_printable_ctlseq(val.data, val.length, ebuf + i, sizeof(ebuf) - i);
        }
        ebuf[i++] = ')';
    }

    LOG_DEBUG (
        "unhandled XTGETTCAP reply: %.*s%.*s",
        (int)len, data, // Original, hex-encoded string (no control chars)
        (int)i, ebuf // Decoded string (with control chars escaped)
    );

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
    if (strview_has_prefix(&seq, ">|")) {
        LOG_INFO("XTVERSION reply: %.*s", (int)len - 2, data + 2);
        return KEY_IGNORE;
    }

    if (strview_has_prefix(&seq, "1+r") || strview_has_prefix(&seq, "0+r")) {
        return parse_xtgettcap_reply(data, len);
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
        LOG_DEBUG("unhandled DECRQSS reply: %.*s", (int)len, data);
        return KEY_IGNORE;
    }

    if (strview_equal_cstring(&seq, "0$r")) {
        note = " (DECRQSS; 'invalid request')";
        goto unhandled;
    }

unhandled:
    LOG_DEBUG("unhandled DCS string%s: %.*s", note, (int)len, data);
    return KEY_IGNORE;
}
