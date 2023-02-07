@@
identifier func;
expression x, y;
binary operator cmp = {<, <=};
position p;
@@

func(...)
{
    <...
-   if ((x) cmp@p (y)) {
-       (x) = (y);
-   }
+   x = MAX(x, y);
    ...>
}

@@
identifier func;
expression x, y;
binary operator cmp = {>, >=};
position p;
@@

func(...)
{
    <...
-   if ((x) cmp@p (y)) {
-       (x) = (y);
-   }
+   x = MIN(x, y);
    ...>
}
