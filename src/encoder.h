#ifndef ENCODER_H
#define ENCODER_H

#include "libc.h"
#include "options.h"

struct file_encoder {
	struct cconv *cconv;
	unsigned char *nbuf;
	ssize_t nsize;
	enum newline_sequence nls;
	int fd;
};

struct file_encoder *new_file_encoder(const char *encoding, enum newline_sequence nls, int fd);
void free_file_encoder(struct file_encoder *enc);
ssize_t file_encoder_write(struct file_encoder *enc, const unsigned char *buf, ssize_t size);

#endif
