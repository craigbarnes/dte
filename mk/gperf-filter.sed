#!/usr/bin/sed -f

4{/^$/i\
/* Filtered by: mk/gperf-filter.sed */
}

/^#if \!((' ' == 32) &&/,/^#endif/d

4,31 {/^$/d}

s/^const \([A-Za-z]*HashSlot\|char\) \*$/static const \1*/

/^\#line/d

/^#ifdef/d
/^#else/d
/^#endif/d
/^__inline/d
