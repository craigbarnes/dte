@@
identifier func;
identifier tail != {
    free,
    ptr_array_free,
    ptr_array_grow,
    search_free_regexp,
    va_end
};
expression lhs, rhs, a1;
@@

func(...)
{
    ...
-   tail(a1);
-   lhs = rhs;
+   lhs = rhs;
+   tail(a1);
}

@@
identifier func;
identifier tail != {string_reserve_space, obuf_reserve_space};
expression lhs, rhs, a1, a2;
@@

func(...)
{
    ...
-   tail(a1, a2);
-   lhs = rhs;
+   lhs = rhs;
+   tail(a1, a2);
}

@@
identifier func != {ptr_array_insert, ptr_array_move, finish_syntax};
identifier tail;
expression lhs, rhs, a1, a2, a3;
@@

func(...)
{
    ...
-   tail(a1, a2, a3);
-   lhs = rhs;
+   lhs = rhs;
+   tail(a1, a2, a3);
}

@@
identifier tail = {memset, memcpy, memmove};
expression dest, a2, a3;
@@

- tail(dest, a2, a3);
- return dest;
+ return tail(dest, a2, a3);

@@
expression false = false;
expression fmt, x, y, z;
@@

(
- error_msg(fmt);
- return false;
+ return error_msg(fmt);
|
- error_msg(fmt, x);
- return false;
+ return error_msg(fmt, x);
|
- error_msg(fmt, x, y);
- return false;
+ return error_msg(fmt, x, y);
|
- error_msg(fmt, x, y, z);
- return false;
+ return error_msg(fmt, x, y, z);
)
