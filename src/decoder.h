#ifndef DECODER_H
#define DECODER_H

#include "libc.h"

struct file_decoder {
	char *encoding;
	const unsigned char *ibuf;
	ssize_t ipos, isize;
	struct cconv *cconv;

	bool (*read_line)(struct file_decoder *dec, char **linep, ssize_t *lenp);
};

struct file_decoder *new_file_decoder(const char *encoding, const unsigned char *buf, ssize_t size);
void free_file_decoder(struct file_decoder *dec);
bool file_decoder_read_line(struct file_decoder *dec, char **line, ssize_t *len);

#endif
