@@
identifier func != {
    ARRAYLEN,
    IS_POWER_OF_2,
    block_iter_is_bol,
    block_iter_is_eof,
    color_is_rgb,
    color_is_valid,
    cond_type_has_destination,
    count_nl,
    cursor_color_is_valid,
    cursor_type_is_valid,
    has_flag,
    is_root_frame,
    is_valid_filetype_name,
    list_empty,
    mem_equal,
    obuf_avail,
    path_is_absolute,
    strlen,
    strview_has_prefix,
    strview_has_suffix
};
@@

/*
 * The following rule matches BUG_ON() assertions that contain function
 * calls, which are best avoided in most cases, since such calls often
 * can't be optimized away (even in non-debug builds). The exceptions
 * listed above were known to be amenable to dead code elimination (i.e.
 * being optimized away) at the time they were added here.
 *
 * See also:
 * - https://codeberg.org/dnkl/foot/issues/330#issuecomment-175276
 * - https://codeberg.org/dnkl/foot/pulls/331#issuecomment-175279
 */

* BUG_ON(<+...func(...)...+>)

@@
identifier func = {
    memcmp,
    memcpy,
    memmove,
    mempcpy,
    strncasecmp,
    strncmp,
    write
};
expression arg1;
@@

/*
 * The STRN() macro shouldn't be used in libc function call arguments,
 * since ISO C also permits them to be function-like macros, which the
 * expanded comma will likely interact with in unexpected ways.
 *
 * See also:
 * - https://gitlab.com/craigbarnes/dte/-/commit/77bec65e5f43ded39239a96cf8c26a5a599c31eb
 * - ISO C99 §7.1.4p1
 */

* func(arg1, <+...STRN(...)...+>)
