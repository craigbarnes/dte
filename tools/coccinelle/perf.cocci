@@
identifier func !~ "divide_equally|add_status_pos|repeat_insert|calculate_tabbar|view_update_vx|report|test_.*";
identifier ident;
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
some form of optimzation (e.g. inlining with constant propagation)
whereby no actual division instructions are generated.
*/

func(...)
{
    <...
*   (x) op@p (ident)
    ...>
}
