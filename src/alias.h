#ifndef ALIAS_H
#define ALIAS_H

#include "util/string.h"

void add_alias(const char *name, const char *value);
void sort_aliases(void);
const char *find_alias(const char *name);
void collect_aliases(const char *prefix);
String dump_aliases(void);

#endif
