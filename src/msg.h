#ifndef MSG_H
#define MSG_H

#include <stdbool.h>
#include <stddef.h>
#include "bookmark.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "view.h"
#include "window.h"

typedef struct {
    FileLocation *loc;
    char msg[];
} Message;

typedef struct {
    PointerArray array;
    size_t pos;
} MessageList;

Message *new_message(const char *msg, size_t len) RETURNS_NONNULL;
void add_message(MessageList *msgs, Message *m) NONNULL_ARGS;
bool activate_current_message(const MessageList *msgs, Window *window) NONNULL_ARGS;
void activate_current_message_save(const MessageList *msgs, PointerArray *bookmarks, const View *view) NONNULL_ARGS;
void clear_messages(MessageList *msgs) NONNULL_ARGS;
String dump_messages(const MessageList *messages) NONNULL_ARGS;

#endif
