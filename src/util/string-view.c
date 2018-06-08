#include <string.h>
#include "string-view.h"

bool string_view_equal(const StringView *a, const StringView *b) {
    return a->length == b->length && !memcmp(a->data, b->data, a->length);
}

bool string_view_equal_cstr(const StringView *sv, const char *cstr) {
    const size_t cstr_len = strlen(cstr);
    return cstr_len == sv->length && !memcmp(sv->data, cstr, cstr_len);
}
