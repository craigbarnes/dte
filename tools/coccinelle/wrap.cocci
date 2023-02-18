@@
identifier func !~ "test_size_increment_wrapped";
expression x, m;
@@

func(...)
{
    <...
-   (x + 1) % m
+   size_increment_wrapped(x, m)
    ...>
}

@@
identifier func !~ "test_size_decrement_wrapped";
expression x, m;
@@

func(...)
{
    <...
-   (x - 1) % m
+   size_decrement_wrapped(x, m)
    ...>
}

@@
expression x, m;
@@

<...
- (x + 1 < m) ? x + 1 : 0
+ size_increment_wrapped(x, m)
...>

@@
expression x, m;
@@

<...
- (x < m - 1) ? x + 1 : 0
+ size_increment_wrapped(x, m)
...>

@@
expression x, m;
@@

<...
- (x ? x : m) - 1
+ size_decrement_wrapped(x, m)
...>
