#ifndef BLOCK_H
#define BLOCK_H

#include "block-iter.h"

Block *block_new(size_t size);
void do_insert(const char *buf, size_t len);
char *do_delete(size_t len);
char *do_replace(size_t del, const char *buf, size_t ins);

#endif
