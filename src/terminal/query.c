#include "query.h"
#include "cursor.h"
#include "terminal.h"
#include "util/ascii.h"
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

static const char *decrpm_mode_to_str(unsigned int mode_param)
{
    switch (mode_param) {
    case 7: return "auto-wrap mode";
    case 25: return "cursor visibility";
    case 45: return "reverse-wraparound mode";
    case 67: return "backspace sends BS";
    case 1036: return "metaSendsEscape";
    case 1039: return "altSendsEscape";
    case 1049: return "alternate screen buffer";
    case 2004: return "bracketed paste";
    case 2026: return "synchronized updates";
    }
    return "unknown";
}

static const char *da2_param_to_name(unsigned int type_param)
{
    switch (type_param) {
    case 0: return "VT100";
    case 1: return "VT220";
    case 2: return "VT240"; // "VT240 or VT241"
    case 18: return "VT330";
    case 19: return "VT340";
    case 24: return "VT320";
    case 32: return "VT382";
    case 41: return "VT420";
    case 61: return "VT510";
    case 64: return "VT520";
    case 65: return "VT525";
    }
    return "unknown";
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

// https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=features%20the%20terminal%20supports
static TermFeatureFlags da1_params_to_features(const TermControlParams *csi)
{
    TermFeatureFlags flags = 0;
    for (size_t i = 1, n = csi->nparams; i < n; i++) {
        unsigned int p = csi->params[i][0];
        if (p == 22) {
            flags |= TFLAG_8_COLOR;
        }
    }
    return flags;
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
    // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=(-,Primary%20DA,-)
    if (prefix == '?' && final == 'c' && intermediate == 0 && nparams >= 1) {
        unsigned int code = csi->params[0][0];
        if (code >= 61 && code <= 65) {
            LOG_INFO("DA1 reply with P=%u (device level %u)", code, code - 60);
            return tflag(TFLAG_QUERY_L2 | TFLAG_QUERY_L3 | da1_params_to_features(csi));
        }
        if (code >= 1 && code <= 12) {
            LOG_INFO("DA1 reply with P=%u (VT100 series)", code);
            return tflag(TFLAG_QUERY_L2);
        }
        LOG_INFO("DA1 reply with P=%u (unknown)", code);
        return KEY_IGNORE;
    }

    // https://vt100.net/docs/vt510-rm/DA2.html
    // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=(-,Secondary%20DA,-)
    if (prefix == '>' && final == 'c' && intermediate == 0 && nparams == 3) {
        unsigned int type = csi->params[0][0];
        unsigned int firmware = csi->params[1][0];
        unsigned int pc = csi->params[2][0];
        const char *name = da2_param_to_name(type);
        LOG_INFO("DA2 reply: %u (%s); %u; %u", type, name, firmware, pc);
        if (type == 0 && firmware == 136 && pc == 0) {
            // This is the DA2 response sent by PuTTY, which doesn't parse
            // DCS sequences in accordance with ECMA-48 and so shouldn't
            // be sent the queries in term_put_level_3_queries()
            return KEY_IGNORE;
        }
        return tflag(TFLAG_QUERY_L3);
    }

    if (prefix == '?' && final == 'y' && intermediate == '$' && nparams == 2) {
        // DECRPM reply to DECRQM query (CSI ? Ps; Pm $ y)
        unsigned int mode = csi->params[0][0];
        unsigned int status = csi->params[1][0];
        const char *mstr = decrpm_mode_to_str(mode);
        const char *sstr = decrpm_status_to_str(status);
        LOG_DEBUG("DECRPM %u (%s) reply: %u (%s)", mode, mstr, status, sstr);
        if (mode == 1036 && status == DECRPM_RESET) {
            return tflag(TFLAG_META_ESC);
        }
        if (mode == 1039 && status == DECRPM_RESET) {
            return tflag(TFLAG_ALT_ESC);
        }
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
    LOG_INFO("unhandled CSI with '%c' param prefix and final byte '%c'", prefix, (char)final);
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

static size_t make_printable_ctlseq(const StringView *seq, char *buf, size_t buflen)
{
    MakePrintableFlags flags = MPF_C0_SYMBOLS;
    return u_make_printable(seq->data, seq->length, buf, buflen, flags);
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

    char ebuf[8 + (U_SET_CHAR_MAXLEN * (sizeof(cbuf) + sizeof(vbuf)))];
    size_t i = 0;
    if (cap.length) {
        i += copyliteral(ebuf + i, " (");
        i += make_printable_ctlseq(&cap, ebuf + i, sizeof(ebuf) - i);
        if (val.length) {
            ebuf[i++] = '=';
            i += make_printable_ctlseq(&val, ebuf + i, sizeof(ebuf) - i);
        }
        ebuf[i++] = ')';
    }

    LOG_INFO (
        "unhandled XTGETTCAP reply: %.*s%.*s",
        (int)len, data, // Original, hex-encoded string (no control chars)
        (int)i, ebuf // Decoded string (with control chars escaped)
    );

    return KEY_IGNORE;
}

static KeyCode handle_decrqss_sgr_reply(const char *data, size_t len)
{
    LOG_DEBUG("DECRQSS SGR reply: %.*s", (int)len, data);

    TermFeatureFlags flags = 0;
    for (size_t pos = 0; pos < len; ) {
        // These SGR params correspond to the ones in term_put_level_3_queries()
        StringView s = get_delim(data, &pos, len, ';');
        if (strview_equal_cstring(&s, "48:5:255")) {
            flags |= TFLAG_256_COLOR;
            continue;
        }

        if (
            strview_equal_cstring(&s, "38:2::60:70:80") // xterm, foot
            || strview_equal_cstring(&s, "38:2:60:70:80") // kitty
        ) {
            flags |= TFLAG_TRUE_COLOR;
            continue;
        }

        if (strview_equal_cstring(&s, "0") && pos <= 2) {
            continue;
        }

        LOG_WARNING (
            "unrecognized parameter substring in DECRQSS SGR reply: %.*s",
            (int)s.length, s.data
        );
    }

    return flags ? tflag(flags) : KEY_IGNORE;
}

KeyCode parse_dcs_query_reply(const char *data, size_t len, bool truncated)
{
    const char *note = "";
    if (unlikely(len == 0 || truncated)) {
        note = truncated ? " (truncated)" : " (empty)";
        goto unhandled;
    }

    StringView seq = string_view(data, len);
    if (strview_remove_matching_prefix(&seq, ">|")) {
        LOG_INFO("XTVERSION reply: %.*s", (int)seq.length, seq.data);
        if (strview_has_prefix(&seq, "tmux ")) {
            // tmux gained support for ECMA-48 REP in version 2.6, but
            // didn't get XTVERSION support until 3.2, so any response
            // starting with "tmux " implies REP support.
            // See also:
            // • https://github.com/tmux/tmux/wiki/FAQ#how-often-is-tmux-released-what-is-the-version-number-scheme
            // • tmux commit 5fc0be50450e75
            // • tmux commit 9dd58470e41bfb
            return tflag(TFLAG_QUERY_L3 | TFLAG_ECMA48_REPEAT);
        }
        return tflag(TFLAG_QUERY_L3);
    }

    // https://vt100.net/docs/vt510-rm/DA3.html
    // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#:~:text=(-,Tertiary%20DA,-)
    if (strview_remove_matching_prefix(&seq, "!|")) {
        LOG_INFO("DA3 reply: %.*s", (int)seq.length, seq.data);
        return tflag(TFLAG_QUERY_L3);
    }

    if (strview_has_prefix(&seq, "1+r") || strview_has_prefix(&seq, "0+r")) {
        return parse_xtgettcap_reply(data, len);
    }

    if (strview_remove_matching_prefix(&seq, "1$r")) {
        if (strview_has_suffix(&seq, " q")) {
            size_t n = seq.length - 2;
            unsigned int x;
            if (n >= 1 && n == buf_parse_uint(seq.data, n, &x)) {
                const char *str = (x <= CURSOR_STEADY_BAR) ? cursor_type_to_str(x) : "??";
                LOG_DEBUG("DECRQSS DECSCUSR (cursor style) reply: %u (%s)", x, str);
                return KEY_IGNORE;
            }
        }

        if (strview_remove_matching_suffix(&seq, "m")) {
            return handle_decrqss_sgr_reply(seq.data, seq.length);
        }

        LOG_INFO("unhandled DECRQSS reply: %.*s", (int)len, data);
        return KEY_IGNORE;
    }

    if (strview_equal_cstring(&seq, "0$r")) {
        note = " (DECRQSS; invalid request)";
        goto unhandled;
    }

unhandled:
    LOG_INFO("unhandled DCS string%s: %.*s", note, (int)len, data);
    return KEY_IGNORE;
}

KeyCode parse_osc_query_reply(const char *data, size_t len, bool truncated)
{
    if (unlikely(len == 0)) {
        return KEY_IGNORE;
    }

    const char *note = truncated ? " (truncated)" : "";
    char prefix = data[0];
    if (prefix == 'L' || prefix == 'l') {
        const char *type = (prefix == 'l') ? "title" : "icon";
        LOG_DEBUG("window %s%s: %.*s", type, note, (int)len - 1, data + 1);
        return KEY_IGNORE;
    }

    LOG_WARNING("unhandled OSC string%s: %.*s", note, (int)len, data);
    return KEY_IGNORE;
}

KeyCode parse_xtwinops_query_reply(const TermControlParams *csi)
{
    if (unlikely(csi->nparams != 3)) {
        LOG_INFO("XTWINOPS sequence with unrecognized number of params");
        return KEY_IGNORE;
    }

    // CSI type ; height ; width t
    const char *typestr = "unknown";
    unsigned int type = csi->params[0][0];
    unsigned int height = csi->params[1][0];
    unsigned int width = csi->params[2][0];

    switch (type) {
    case 4: typestr = "text area size in pixels"; break;
    case 6: typestr = "cell size in pixels"; break;
    case 8: typestr = "text area size in \"characters\""; break;
    }

    LOG_INFO("XTWINOPS %u (%s) reply: %ux%u", type, typestr, width, height);
    return KEY_IGNORE;
}
