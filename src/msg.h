#ifndef MSG_H
#define MSG_H

#include <stddef.h>
#include "util/macros.h"

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

FileLocation *file_location_create (
    const char *filename,
    unsigned long buffer_id,
    unsigned long line,
    unsigned long column
);

void file_location_free(FileLocation *loc);
void push_file_location(FileLocation *loc);
void pop_file_location(void);

Message *new_message(const char *msg);
void add_message(Message *m);
void activate_current_message(void);
void activate_next_message(void);
void activate_prev_message(void);
void clear_messages(void);
size_t message_count(void) PURE;

#endif
