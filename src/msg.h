#ifndef MSG_H
#define MSG_H

#include <stddef.h>
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string.h"
#include "view.h"

typedef struct {
    // Needed after buffer is closed
    char *filename;

    // Needed if buffer doesn't have filename.
    // Pointer would have to be set to NULL after closing buffer.
    unsigned long buffer_id;

    // If pattern is set then line and column are 0 and vice versa
    char *pattern; // Regex from tag file
    unsigned long line, column;
} FileLocation;

typedef struct {
    char *msg;
    FileLocation *loc;
} Message;

typedef struct {
    PointerArray array;
    size_t pos;
} MessageArray;

FileLocation *get_current_file_location(const View *view) NONNULL_ARGS_AND_RETURN;
void file_location_free(FileLocation *loc);
void push_file_location(PointerArray *locations, FileLocation *loc);
void pop_file_location(PointerArray *locations);

Message *new_message(const char *msg, size_t len) RETURNS_NONNULL;
void add_message(MessageArray *arr, Message *m);
void activate_message(MessageArray *arr, size_t idx);
void activate_current_message(const MessageArray *arr);
void activate_next_message(MessageArray *arr);
void activate_prev_message(MessageArray *arr);
void activate_current_message_save(const MessageArray *arr, PointerArray *file_locations, const View *view) NONNULL_ARGS;
void clear_messages(MessageArray *arr);
String dump_messages(const MessageArray *messages);

#endif
