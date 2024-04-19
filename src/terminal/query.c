#include "query.h"
#include "cursor.h"
#include "terminal.h"
#include "util/log.h"
#include "util/string-view.h"
#include "util/strtonum.h"

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
