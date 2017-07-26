#ifndef MSG_H
#define MSG_H

#include "libc.h"
#include "file-location.h"

struct message {
	char *msg;
	struct file_location *loc;
};

struct message *new_message(const char *msg);
void add_message(struct message *m);
void activate_current_message(void);
void activate_next_message(void);
void activate_prev_message(void);
void clear_messages(void);
int message_count(void);

#endif
