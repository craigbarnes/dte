#ifndef ENCODING_H
#define ENCODING_H

#include "libc.h"

struct byte_order_mark {
	const char *encoding;
	unsigned char bytes[4];
	int len;
};

char *normalize_encoding(const char *encoding);
const struct byte_order_mark *get_bom_for_encoding(const char *encoding);
const char *detect_encoding_from_bom(const unsigned char *buf, size_t size);

#endif
