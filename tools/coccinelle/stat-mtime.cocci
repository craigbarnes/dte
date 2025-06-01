@@
identifier func != get_stat_mtime;
struct stat *st;
@@

func(...)
{
    <...
-   st->st_mtim
+   get_stat_mtime(st)
    ...>
}
