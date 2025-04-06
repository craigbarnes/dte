@@
identifier func != {
    add_status_pos,
    bisearch_idx,
    calculate_tabbar,
    divide_equally,
    human_readable_size,
    indent_level,
    indent_remainder,
    next_indent_width,
    repeat_insert,
    report,
    test_indent_level,
    test_next_indent_width,
    test_size_decrement_wrapped,
    test_size_increment_wrapped,
    view_update_vx
};
identifier ident !~ "^[A-Z][A-Z0-9_]+$";
expression x;
binary operator op = {/, %};
position p;
@@

/*
This rule finds divide and modulo operations where the right hand
side is an identifier (i.e. not a numeric literal). Such operations
should ideally be avoided, where possible, for performance reasons.

The exceptions listed above are either deemed acceptable uses of
division that can't be optimized away or are known to be amenable to
some form of optimization (e.g. inlining with constant propagation)
whereby no actual division instructions are generated.
*/

func(...)
{
    <...
*   (x) op@p (ident)
    ...>
}
