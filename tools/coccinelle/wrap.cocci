@@
identifier func != test_wrapping_increment;
expression x, m;
@@

func(...)
{
    <...
-   (x + 1) % m
+   wrapping_increment(x, m)
    ...>
}

@@
identifier func != test_wrapping_decrement;
expression x, m;
@@

func(...)
{
    <...
-   (x - 1) % m
+   wrapping_decrement(x, m)
    ...>
}

@@
expression x, m;
@@

<...
- (x + 1 < m) ? x + 1 : 0
+ wrapping_increment(x, m)
...>

@@
expression x, m;
@@

<...
- (x < m - 1) ? x + 1 : 0
+ wrapping_increment(x, m)
...>

@@
expression x, m;
@@

<...
- (x ? x : m) - 1
+ wrapping_decrement(x, m)
...>
