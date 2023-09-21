@include@
@@

#include "util/macros.h"

@depends on include@
type T;
T[] E;
@@

(
- (sizeof(E) / sizeof(*E))
+ ARRAYLEN(E)
|
- (sizeof(E) / sizeof(E[...]))
+ ARRAYLEN(E)
|
- (sizeof(E) / sizeof(T))
+ ARRAYLEN(E)
)
