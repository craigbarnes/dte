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
    const unsigned char NONSTRING *data;
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

static inline StringView strview_from_cstring(const char *str)
{
    return string_view(str, str ? strlen(str) : 0);
}

static inline StringView strview(const char *str)
{
    return strview_from_cstring(str);
}

NONNULL_ARGS
static inline bool strview_equal(const StringView *a, const StringView *b)
{
    size_t n = a->length;
    return n == b->length && mem_equal(a->data, b->data, n);
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
static inline bool strview_equal_strn_icase (
    const StringView *sv,
    const char *str,
    size_t len
) {
    return len == sv->length && mem_equal_icase(sv->data, str, len);
}

NONNULL_ARGS
static inline bool strview_equal_cstring(const StringView *sv, const char *str)
{
    return strview_equal_strn(sv, str, strlen(str));
}

NONNULL_ARGS
static inline bool strview_equal_cstring_icase(const StringView *sv, const char *str)
{
    return strview_equal_strn_icase(sv, str, strlen(str));
}

NONNULL_ARG(1) NONNULL_ARG_IF_NONZERO_LENGTH(2, 3)
static inline bool strview_has_strn_prefix(const StringView *sv, const char *p, size_t n)
{
    return sv->length >= n && mem_equal(sv->data, p, n);
}

NONNULL_ARG(1) NONNULL_ARG_IF_NONZERO_LENGTH(2, 3)
static inline bool strview_has_strn_suffix(const StringView *sv, const char *suf, size_t suflen)
{
    // See also: https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3322.pdf
    size_t len = sv->length;
    return suflen == 0 || (len >= suflen && mem_equal(sv->data + len - suflen, suf, suflen));
}

NONNULL_ARG(1) NONNULL_ARG_IF_NONZERO_LENGTH(2, 3) NONNULL_ARG_IF_NONZERO_LENGTH(4, 5)
static inline bool strview_has_strn_prefix_and_suffix (
    const StringView *sv,
    const char *prefix, size_t prefix_len,
    const char *suffix, size_t suffix_len
) {
    return prefix_len + suffix_len <= sv->length
        && strview_has_strn_prefix(sv, prefix, prefix_len)
        && strview_has_strn_suffix(sv, suffix, suffix_len);
}

NONNULL_ARGS
static inline bool strview_has_prefix(const StringView *sv, const char *p)
{
    return strview_has_strn_prefix(sv, p, strlen(p));
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
    return strview_has_strn_prefix(sv, pfx1, strlen(pfx1))
        || strview_has_strn_prefix(sv, pfx2, strlen(pfx2));
}

NONNULL_ARGS
static inline bool strview_has_suffix(const StringView *sv, const char *suffix)
{
    return strview_has_strn_suffix(sv, suffix, strlen(suffix));
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

NONNULL_ARGS
static inline bool strview_contains_char_type(const StringView *sv, AsciiCharType mask)
{
    return strn_contains_ascii_char_type(sv->data, sv->length, mask);
}

NONNULL_ARGS
static inline const unsigned char *strview_memchr(const StringView *sv, int c)
{
    return sv->length ? memchr(sv->data, c, sv->length) : NULL;
}

NONNULL_ARGS
static inline const unsigned char *strview_memrchr(const StringView *sv, int c)
{
    return sv->length ? xmemrchr(sv->data, c, sv->length) : NULL;
}

NONNULL_ARGS
static inline ssize_t strview_memrchr_idx(const StringView *sv, int c)
{
    const unsigned char *ptr = strview_memrchr(sv, c);
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

static inline bool strview_remove_matching_strn_prefix (
    StringView *sv,
    const char *prefix,
    size_t prefix_len
) {
    if (!strview_has_strn_prefix(sv, prefix, prefix_len)) {
        return false;
    }
    strview_remove_prefix(sv, prefix_len);
    return true;
}

static inline bool strview_remove_matching_prefix(StringView *sv, const char *prefix)
{
    return strview_remove_matching_strn_prefix(sv, prefix, strlen(prefix));
}

static inline bool strview_remove_matching_suffix(StringView *sv, const char *suffix)
{
    size_t suffix_len = strlen(suffix);
    if (!strview_has_strn_suffix(sv, suffix, suffix_len)) {
        return false;
    }
    sv->length -= suffix_len;
    return true;
}

static inline bool strview_remove_matching_prefix_and_suffix (
    StringView *sv,
    const char *prefix,
    const char *suffix
) {
    size_t plen = strlen(prefix);
    size_t slen = strlen(suffix);
    if (strview_has_strn_prefix_and_suffix(sv, prefix, plen, suffix, slen)) {
        strview_remove_prefix(sv, plen);
        strview_remove_suffix(sv, slen);
        return true;
    }
    return false;
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
    const unsigned char *data = sv->data;
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
