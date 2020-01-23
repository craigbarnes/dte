#ifndef LOCK_H
#define LOCK_H

#include <stdbool.h>

bool lock_file(const char *filename);
void unlock_file(const char *filename);

#endif
