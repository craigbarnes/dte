#ifndef BLOCK_H
#define BLOCK_H

struct block *block_new(long size);
void do_insert(const char *buf, long len);
char *do_delete(long len);
char *do_replace(long del, const char *buf, long ins);

#endif
