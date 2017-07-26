#include "encoder.h"
#include "uchar.h"
#include "common.h"
#include "cconv.h"

struct file_encoder *new_file_encoder(const char *encoding, enum newline_sequence nls, int fd)
{
	struct file_encoder *enc = xnew0(struct file_encoder, 1);

	enc->nls = nls;
	enc->fd = fd;

	if (!streq(encoding, "UTF-8")) {
		enc->cconv = cconv_from_utf8(encoding);
		if (enc->cconv == NULL) {
			free(enc);
			return NULL;
		}
	}
	return enc;
}

void free_file_encoder(struct file_encoder *enc)
{
	if (enc->cconv != NULL)
		cconv_free(enc->cconv);
	free(enc->nbuf);
	free(enc);
}

static ssize_t unix_to_dos(struct file_encoder *enc, const unsigned char *buf, ssize_t size)
{
	ssize_t s, d;

	if (enc->nsize < size * 2) {
		enc->nsize = size * 2;
		xrenew(enc->nbuf, enc->nsize);
	}

	for (s = 0, d = 0; s < size; s++) {
		unsigned char ch = buf[s];
		if (ch == '\n')
			enc->nbuf[d++] = '\r';
		enc->nbuf[d++] = ch;
	}
	return d;
}

// NOTE: buf must contain whole characters!
ssize_t file_encoder_write(struct file_encoder *enc, const unsigned char *buf, ssize_t size)
{
	if (enc->nls == NEWLINE_DOS) {
		size = unix_to_dos(enc, buf, size);
		buf = enc->nbuf;
	}

	if (enc->cconv == NULL)
		return xwrite(enc->fd, buf, size);

	cconv_process(enc->cconv, buf, size);
	cconv_flush(enc->cconv);
	buf = cconv_consume_all(enc->cconv, &size);
	return xwrite(enc->fd, buf, size);
}
