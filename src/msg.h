#ifndef MSG_H
#define MSG_H

typedef struct {
    // Needed after buffer is closed
    char *filename;

    // Needed if buffer doesn't have filename.
    // Pointer would have to be set to NULL after closing buffer.
    unsigned int buffer_id;

    // If pattern is set then line and column are 0 and vice versa
    char *pattern; // Regex from tag file
    int line, column;
} FileLocation;

typedef struct {
    char *msg;
    FileLocation *loc;
} Message;

struct View;

FileLocation *create_file_location(const struct View *v);
void file_location_free(FileLocation *loc);
void push_file_location(FileLocation *loc);
void pop_file_location(void);

Message *new_message(const char *msg);
void add_message(Message *m);
void activate_current_message(void);
void activate_next_message(void);
void activate_prev_message(void);
void clear_messages(void);
int message_count(void);

#endif
