#ifndef MSG_H
#define MSG_H

#include <stdbool.h>
#include <stddef.h>
#include "bookmark.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"

typedef struct {
    FileLocation *loc;
    char msg[];
} Message;

typedef struct {
    PointerArray array;
    size_t pos;
} MessageArray;

struct EditorState;

Message *new_message(const char *msg, size_t len) RETURNS_NONNULL;
void add_message(MessageArray *msgs, Message *m) NONNULL_ARGS;
bool activate_current_message(struct EditorState *e) NONNULL_ARGS;
bool activate_current_message_save(struct EditorState *e) NONNULL_ARGS;
void clear_messages(MessageArray *msgs) NONNULL_ARGS;
String dump_messages(const MessageArray *messages) NONNULL_ARGS;

#endif
