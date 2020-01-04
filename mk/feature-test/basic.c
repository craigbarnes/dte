/*
 This file serves as a minimal, valid input that can be passed to
 a compiler in order to make it run in C mode (like "gcc -xc").

 The behaviour for a file argument matching "*.c" is specified by
 the POSIX c99(1) spec, whereas "-xc" is a GCC-specific flag.

 See also:
   - https://pubs.opengroup.org/onlinepubs/9699919799/utilities/c99.html
   - https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html
*/

int main(void)
{
    return 0;
}
