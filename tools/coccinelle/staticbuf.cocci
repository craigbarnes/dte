@disable optional_qualifier@
identifier func != {
    fsize_string,
    lines_and_columns_env,
    umax_to_str
};
identifier buf;
@@

func(...)
{
    <...
*   static char buf[...];
    ...>
}

@smbuf disable optional_qualifier@
identifier buf != {error_buf, dim, sgr0};
position p;
@@

static@p char buf[...];

@script:python@
p << smbuf.p;
buf << smbuf.buf;
@@

x = p[0]
if x.column == "0":
    print("%s:%s:%s: static, mutable buffer: %s[]" % (x.file, x.line, x.column, buf))
