#ifndef CCONV_H
#define CCONV_H

#include <stddef.h>

struct cconv;

struct cconv *cconv_to_utf8(const char *encoding);
struct cconv *cconv_from_utf8(const char *encoding);
void cconv_process(struct cconv *c, const char *input, size_t len);
void cconv_flush(struct cconv *c);
size_t cconv_nr_errors(const struct cconv *c);
char *cconv_consume_line(struct cconv *c, size_t *len);
char *cconv_consume_all(struct cconv *c, size_t *len);
void cconv_free(struct cconv *c);

#endif
