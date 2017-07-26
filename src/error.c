#include "error.h"
#include "editor.h"
#include "config.h"
#include "common.h"

int nr_errors;
bool msg_is_error;
char error_buf[256];

static struct error *error_new(char *msg)
{
	struct error *err = xnew0(struct error, 1);
	err->msg = msg;
	return err;
}

struct error *error_create(const char *format, ...)
{
	struct error *err;
	va_list ap;

	va_start(ap, format);
	err = error_new(xvsprintf(format, ap));
	va_end(ap);
	return err;
}

struct error *error_create_errno(int code, const char *format, ...)
{
	struct error *err;
	va_list ap;

	va_start(ap, format);
	err = error_new(xvsprintf(format, ap));
	va_end(ap);
	err->code = code;
	return err;
}

void error_free(struct error *err)
{
	if (err != NULL) {
		free(err->msg);
		free(err);
	}
}

void clear_error(void)
{
	error_buf[0] = 0;
}

void error_msg(const char *format, ...)
{
	va_list ap;
	int pos = 0;

	// some implementations of *printf return -1 if output was truncated
	if (config_file) {
		snprintf(error_buf, sizeof(error_buf), "%s:%d: ", config_file, config_line);
		pos = strlen(error_buf);
		if (current_command) {
			snprintf(error_buf + pos, sizeof(error_buf) - pos,
				"%s: ", current_command->name);
			pos += strlen(error_buf + pos);
		}
	}

	va_start(ap, format);
	vsnprintf(error_buf + pos, sizeof(error_buf) - pos, format, ap);
	va_end(ap);

	msg_is_error = true;
	nr_errors++;

	if (editor_status != EDITOR_RUNNING)
		fprintf(stderr, "%s\n", error_buf);
}

void info_msg(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsnprintf(error_buf, sizeof(error_buf), format, ap);
	va_end(ap);
	msg_is_error = false;
}
