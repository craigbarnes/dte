@@
identifier func;
identifier tail !~ "va_end|.*free.*";
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
identifier tail !~ "string_reserve_space|obuf_need_space";
expression lhs !~ "current_config";
expression rhs, a1, a2;
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
identifier func !~ "ptr_array_insert|ptr_array_move|finish_syntax";
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
