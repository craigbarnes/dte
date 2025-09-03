#ifndef UTIL_STRING_VIEW_H
#define UTIL_STRING_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include "ascii.h"
#include "debug.h"
#include "macros.h"
#include "xmemrchr.h"
#include "xstring.h"

// A non-owning, length-bounded "view" into another string, similar to
// the C++17 string_view class or what many languages call a "slice".
// The .data member will usually *not* be null-terminated and the
// underlying string *must* outlive the view.
typedef struct {
    const char NONSTRING *data;
    size_t length;
} StringView;

#define STRING_VIEW_INIT { \
    .data = NULL, \
    .length = 0 \
}

#define STRING_VIEW(s) { \
    .data = s, \
    .length = STRLEN(s) \
}

static inline StringView string_view(const char *str, size_t length)
{
    return (StringView) {
        .data = str,
        .length = length
    };
}

static inline StringView strview(const char *str)
{
    return string_view(str, str ? strlen(str) : 0);
}

static inline bool strview_equal(StringView a, StringView b)
{
    size_t n = a.length;
    return n == b.length && mem_equal(a.data, b.data, n);
}

static inline bool strview_equal_icase(StringView a, StringView b)
{
    size_t n = a.length;
    return n == b.length && mem_equal_icase(a.data, b.data, n);
}

NONNULL_ARGS
static inline bool strview_equal_strn (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && mem_equal(sv->data, str, len);
}

NONNULL_ARGS
static inline bool strview_equal_cstring(const StringView *sv, const char *str)
{
    return strview_equal(*sv, strview(str));
}

static inline bool strview_has_sv_prefix(StringView sv, StringView prefix)
{
    const size_t plen = prefix.length;
    return sv.length >= plen && mem_equal(sv.data, prefix.data, plen);
}

static inline bool strview_has_sv_suffix(StringView sv, StringView suffix)
{
    size_t len = sv.length;
    size_t suflen = suffix.length;
    return len >= suflen && mem_equal(sv.data + len - suflen, suffix.data, suflen);
}

NONNULL_ARGS
static inline bool strview_has_prefix(const StringView *sv, const char *p)
{
    return strview_has_sv_prefix(*sv, strview(p));
}

NONNULL_ARGS
static inline bool strview_has_prefix_icase(const StringView *sv, const char *p)
{
    size_t length = strlen(p);
    return sv->length >= length && mem_equal_icase(sv->data, p, length);
}

NONNULL_ARGS
static inline bool strview_has_either_prefix (
    const StringView *sv,
    const char *pfx1,
    const char *pfx2
) {
    return strview_has_sv_prefix(*sv, strview(pfx1))
        || strview_has_sv_prefix(*sv, strview(pfx2));
}

NONNULL_ARGS
static inline bool strview_has_suffix(const StringView *sv, const char *suffix)
{
    return strview_has_sv_suffix(*sv, strview(suffix));
}

NONNULL_ARGS
static inline bool strview_has_prefix_and_suffix (
    const StringView *sv,
    const char *prefix,
    const char *suffix
) {
    return strview_has_prefix(sv, prefix) && strview_has_suffix(sv, suffix);
}

NONNULL_ARGS
static inline bool strview_isblank(const StringView *sv)
{
    size_t len = sv->length;
    return len == ascii_blank_prefix_length(sv->data, len);
}

static inline bool strview_contains_char_type(StringView sv, AsciiCharType mask)
{
    for (size_t i = 0, n = sv.length; i < n; i++) {
        if (ascii_test(sv.data[i], mask)) {
            return true;
        }
    }
    return false;
}

NONNULL_ARGS
static inline const char *strview_memchr(const StringView *sv, int c)
{
    return sv->length ? memchr(sv->data, c, sv->length) : NULL;
}

NONNULL_ARGS
static inline const char *strview_memrchr(const StringView *sv, int c)
{
    return sv->length ? xmemrchr(sv->data, c, sv->length) : NULL;
}

NONNULL_ARGS
static inline ssize_t strview_memrchr_idx(const StringView *sv, int c)
{
    const char *ptr = strview_memrchr(sv, c);
    return ptr ? (ssize_t)(ptr - sv->data) : -1;
}

static inline void strview_remove_prefix(StringView *sv, size_t len)
{
    BUG_ON(len > sv->length);
    if (len > 0) {
        sv->data += len;
        sv->length -= len;
    }
}

static inline void strview_remove_suffix(StringView *sv, size_t len)
{
    BUG_ON(len > sv->length);
    sv->length -= len;
}

static inline bool strview_remove_matching_sv_prefix (
    StringView *sv,
    StringView prefix
) {
    bool match = strview_has_sv_prefix(*sv, prefix);
    strview_remove_prefix(sv, match ? prefix.length : 0);
    return match;
}

static inline bool strview_remove_matching_sv_suffix (
    StringView *sv,
    StringView suffix
) {
    bool match = strview_has_sv_suffix(*sv, suffix);
    strview_remove_suffix(sv, match ? suffix.length : 0);
    return match;
}

static inline bool strview_remove_matching_prefix(StringView *sv, const char *prefix)
{
    return strview_remove_matching_sv_prefix(sv, strview(prefix));
}

static inline bool strview_remove_matching_suffix(StringView *sv, const char *suffix)
{
    return strview_remove_matching_sv_suffix(sv, strview(suffix));
}

static inline bool strview_remove_matching_affixes (
    StringView *sv,
    StringView prefix,
    StringView suffix
) {
    size_t total_affix_length = prefix.length + suffix.length;
    bool pmatch = strview_has_sv_prefix(*sv, prefix);
    bool smatch = strview_has_sv_suffix(*sv, suffix);
    bool match = (total_affix_length <= sv->length) && pmatch && smatch;

    if (match) {
        sv->data += prefix.length;
        sv->length -= total_affix_length;
    }

    return match;
}

NONNULL_ARGS
static inline size_t strview_trim_left(StringView *sv)
{
    size_t blank_len = ascii_blank_prefix_length(sv->data, sv->length);
    strview_remove_prefix(sv, blank_len);
    return blank_len;
}

NONNULL_ARGS
static inline void strview_trim_right(StringView *sv)
{
    const char *data = sv->data;
    size_t n = sv->length;
    while (n && ascii_isblank(data[n - 1])) {
        n--;
    }
    sv->length = n;
}

NONNULL_ARGS
static inline void strview_trim(StringView *sv)
{
    strview_trim_left(sv);
    strview_trim_right(sv);
}

#endif
