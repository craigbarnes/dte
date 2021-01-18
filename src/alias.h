#ifndef ALIAS_H
#define ALIAS_H

#include "util/string.h"

void init_aliases(void);
void add_alias(const char *name, const char *value);
const char *find_alias(const char *name);
void collect_aliases(const char *prefix);
String dump_aliases(void);

#endif
