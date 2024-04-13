@@
identifier func = {
    memcmp,
    memcpy,
    memmove,
    mempcpy,
    strncasecmp,
    strncmp,
    write
};
expression arg1;
@@

/*
 * The STRN() macro shouldn't be used in libc function call arguments,
 * since ISO C also permits them to be function-like macros, which the
 * expanded comma will likely interact with in unexpected ways.
 *
 * See also:
 * • https://gitlab.com/craigbarnes/dte/-/commit/77bec65e5f43ded39239a96cf8c26a5a599c31eb
 * • ISO C99 §7.1.4p1
 */

* func(arg1, <+...STRN(...)...+>)

@@
identifier main = main;
identifier argv = argv;
@@

// In main(), progname() should be used instead of argv[0]
main(...)
{
    <...
*   argv[0]
    ...>
}
