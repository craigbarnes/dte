#ifndef MSG_H
#define MSG_H

#include "file-location.h"

typedef struct {
    char *msg;
    FileLocation *loc;
} Message;

Message *new_message(const char *msg);
void add_message(Message *m);
void activate_current_message(void);
void activate_next_message(void);
void activate_prev_message(void);
void clear_messages(void);
int message_count(void);

#endif
