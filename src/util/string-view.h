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

#define SV(s) STRING_VIEW(s)

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

// Return a view of the `str` substring between `start` and `end`.
// This is similar to xstrslice(), but without the need to allocate,
// copy or null-terminate.
static inline StringView strview_from_slice(const char *str, size_t start, size_t end)
{
    BUG_ON(start > end);
    BUG_ON(end && !str);
    return string_view(start ? str + start : str, end - start);
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

static inline bool strview_equal_cstring(StringView sv, const char *str)
{
    return strview_equal(sv, strview(str));
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

static inline bool strview_has_prefix(StringView sv, const char *prefix)
{
    return strview_has_sv_prefix(sv, strview(prefix));
}

static inline bool strview_has_prefix_icase(StringView sv, const char *prefix)
{
    size_t length = strlen(prefix);
    return sv.length >= length && mem_equal_icase(sv.data, prefix, length);
}

static inline bool strview_has_either_prefix (
    StringView sv,
    const char *pfx1,
    const char *pfx2
) {
    return strview_has_sv_prefix(sv, strview(pfx1))
        || strview_has_sv_prefix(sv, strview(pfx2));
}

static inline bool strview_has_suffix(StringView sv, const char *suffix)
{
    return strview_has_sv_suffix(sv, strview(suffix));
}

static inline bool strview_has_sv_prefix_and_suffix (
    StringView sv,
    StringView prefix,
    StringView suffix
) {
    return
        sv.length >= prefix.length + suffix.length
        && strview_has_sv_prefix(sv, prefix)
        && strview_has_sv_suffix(sv, suffix)
    ;
}

static inline bool strview_has_prefix_and_suffix (
    StringView sv,
    const char *prefix,
    const char *suffix
) {
    return strview_has_sv_prefix_and_suffix(sv, strview(prefix), strview(suffix));
}

static inline size_t strview_blank_prefix_length(StringView sv)
{
    size_t i = 0;
    while (i < sv.length && ascii_isblank(sv.data[i])) {
        i++;
    }
    return i;
}

static inline size_t strview_blank_suffix_length(StringView sv)
{
    size_t n = sv.length;
    while (n && ascii_isblank(sv.data[n - 1])) {
        n--;
    }
    return sv.length - n;
}

static inline bool strview_isblank(StringView sv)
{
    return strview_blank_prefix_length(sv) == sv.length;
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

static inline const char *strview_memchr(StringView sv, int c)
{
    return sv.length ? memchr(sv.data, c, sv.length) : NULL;
}

static inline const char *strview_memrchr(StringView sv, int c)
{
    return sv.length ? xmemrchr(sv.data, c, sv.length) : NULL;
}

static inline ssize_t strview_memrchr_idx(StringView sv, int c)
{
    const char *ptr = strview_memrchr(sv, c);
    return ptr ? (ssize_t)(ptr - sv.data) : -1;
}

static inline size_t strview_remove_prefix(StringView *sv, size_t len)
{
    BUG_ON(len > sv->length);
    sv->data = len ? sv->data + len : sv->data;
    sv->length -= len;
    return len;
}

static inline size_t strview_remove_suffix(StringView *sv, size_t len)
{
    BUG_ON(len > sv->length);
    sv->length -= len;
    return len;
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
    return strview_remove_prefix(sv, strview_blank_prefix_length(*sv));
}

NONNULL_ARGS
static inline size_t strview_trim_right(StringView *sv)
{
    return strview_remove_suffix(sv, strview_blank_suffix_length(*sv));
}

NONNULL_ARGS
static inline size_t strview_trim(StringView *sv)
{
    size_t r = strview_trim_right(sv);
    return r + strview_trim_left(sv);
}

#endif
