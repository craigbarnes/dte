#ifndef MSG_H
#define MSG_H

#include <stddef.h>
#include "bookmark.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "view.h"

typedef struct {
    FileLocation *loc;
    char msg[];
} Message;

typedef struct {
    PointerArray array;
    size_t pos;
} MessageArray;

Message *new_message(const char *msg, size_t len) RETURNS_NONNULL;
void add_message(MessageArray *msgs, Message *m);
void activate_message(MessageArray *msgs, size_t idx);
void activate_current_message(const MessageArray *msgs);
void activate_next_message(MessageArray *msgs);
void activate_prev_message(MessageArray *msgs);
void activate_current_message_save(const MessageArray *msgs, PointerArray *file_locations, const View *view) NONNULL_ARGS;
void clear_messages(MessageArray *msgs);
String dump_messages(const MessageArray *messages);

#endif
