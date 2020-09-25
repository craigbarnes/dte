#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <string.h>
#include "macros.h"

#define BUG_ON(a) do { \
    IGNORE_WARNING("-Wtautological-compare") \
    if (unlikely(a)) { \
        BUG("%s", #a); \
    } \
    UNIGNORE_WARNINGS \
} while (0)

#if DEBUG >= 1
    #define BUG(...) bug(__FILE__, __LINE__, __func__, __VA_ARGS__)
    noreturn void bug(const char *file, int line, const char *func, const char *fmt, ...) COLD PRINTF(4);
#else
    #define BUG(...) UNREACHABLE()
#endif

#if DEBUG >= 2
    #define DEBUG_LOG(...) debug_log(__func__, __VA_ARGS__)
    void debug_log(const char *function, const char *fmt, ...) PRINTF(2);
    void log_init(const char *varname);
#else
    static inline PRINTF(1) void DEBUG_LOG(const char* UNUSED_ARG(fmt), ...) {}
    static inline void log_init(const char* UNUSED_ARG(varname)) {}
#endif

#if DEBUG >= 1 && defined(HAS_TYPEOF)
    #define CHECK_BSEARCH_ARRAY(arr, field) do { \
        static_assert_compatible_types(arr[0].field[0], char); \
        check_bsearch_array ( \
            arr, \
            #arr, \
            #field, \
            ARRAY_COUNT(arr), \
            sizeof(arr[0]), \
            offsetof(__typeof__(*arr), field), \
            sizeof(arr[0].field) \
        ); \
    } while (0)

    #define CHECK_BSEARCH_STR_ARRAY(arr) do { \
        static_assert_compatible_types(arr[0][0], char); \
        check_bsearch_array ( \
            arr, \
            #arr, \
            "", \
            ARRAY_COUNT(arr), \
            sizeof(arr[0]), \
            0, \
            sizeof(arr[0]) \
        ); \
    } while (0)
#else
    #define CHECK_BSEARCH_ARRAY(arr, field)
    #define CHECK_BSEARCH_STR_ARRAY(arr)
#endif

static inline void check_bsearch_array (
    const void *array_base,
    const char *array_name,
    const char *name_field_name,
    size_t array_length,
    size_t array_element_size,
    size_t name_offset,
    size_t name_size
) {
    BUG_ON(!array_base);
    BUG_ON(!array_name);
    BUG_ON(!name_field_name);
    BUG_ON(array_length == 0);
    BUG_ON(array_element_size == 0);
    BUG_ON(name_size == 0);

    const char *first_name = array_base + name_offset;
    if (first_name[name_size - 1] != '\0') {
        BUG("String sentinel missing from %s[0].%s", array_name, name_field_name);
    }

    for (size_t i = 1; i < array_length; i++) {
        const char *curr_name = array_base + (i * array_element_size) + name_offset;
        const char *prev_name = curr_name - array_element_size;
        if (curr_name[name_size - 1] != '\0') {
            BUG("String sentinel missing from %s[%zu].%s", array_name, i, name_field_name);
        }
        if (strcmp(curr_name, prev_name) <= 0) {
            BUG (
                "String at %s[%zu].%s not in sorted order: \"%s\" (prev: \"%s\")",
                array_name, i, name_field_name,
                curr_name, prev_name
            );
        }
    }
}

noreturn void fatal_error(const char *msg, int err) COLD NONNULL_ARGS;
void set_fatal_error_cleanup_handler(void (*handler)(void));

#endif
