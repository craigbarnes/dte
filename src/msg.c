#include "msg.h"
#include "ptr-array.h"
#include "error.h"
#include "common.h"

static PTR_ARRAY(msgs);
static int msg_pos;

static void free_message(struct message *m)
{
	free(m->msg);
	if (m->loc != NULL) {
		file_location_free(m->loc);
	}
	free(m);
}

static bool message_equals(const struct message *a, const struct message *b)
{
	if (!streq(a->msg, b->msg)) {
		return false;
	}
	if (a->loc == NULL) {
		return b->loc == NULL;
	}
	if (b->loc == NULL) {
		return false;
	}
	return file_location_equals(a->loc, b->loc);
}

static bool is_duplicate(const struct message *m)
{
	int i;

	for (i = 0; i < msgs.count; i++) {
		if (message_equals(m, msgs.ptrs[i])) {
			return true;
		}
	}
	return false;
}

struct message *new_message(const char *msg)
{
	struct message *m = xnew0(struct message, 1);
	m->msg = xstrdup(msg);
	return m;
}

void add_message(struct message *m)
{
	if (is_duplicate(m)) {
		free_message(m);
	} else {
		ptr_array_add(&msgs, m);
	}
}

void activate_current_message(void)
{
	struct message *m;

	if (msg_pos == msgs.count) {
		return;
	}
	m = msgs.ptrs[msg_pos];
	if (m->loc != NULL) {
		if (!file_location_go(m->loc)) {
			// error message is visible
			return;
		}
	}
	info_msg("[%d/%ld] %s", msg_pos + 1, msgs.count, m->msg);
}

void activate_next_message(void)
{
	if (msg_pos + 1 < msgs.count)
		msg_pos++;
	activate_current_message();
}

void activate_prev_message(void)
{
	if (msg_pos > 0)
		msg_pos--;
	activate_current_message();
}

void clear_messages(void)
{
	ptr_array_free_cb(&msgs, FREE_FUNC(free_message));
	msg_pos = 0;
}

int message_count(void)
{
	return msgs.count;
}
