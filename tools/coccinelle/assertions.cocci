@@
identifier f !~ "strlen|ARRAYLEN|IS_POWER_OF_2|has_flag|cond_type_has_destination|count_nl|list_empty|mem_equal|block_iter_is_bol|block_iter_is_eof|is_valid_filetype_name|path_is_absolute|obuf_avail|strview_has_prefix|strview_has_suffix";
@@

/*
The following rule matches BUG_ON() assertions that contain function
calls, which are best avoided in most cases, since such calls often
can't be optimized away (even in non-debug builds). The exceptions
listed above were known to be amenable to dead code elimination (i.e.
being optimized away) at the time they were added here.

See also: https://codeberg.org/dnkl/foot/issues/330#issuecomment-175276
*/

* BUG_ON(<+...f(...)...+>)
