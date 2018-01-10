#ifndef FILE_LOCATION_H
#define FILE_LOCATION_H

#include <stdbool.h>

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

struct View;

FileLocation *create_file_location(const struct View *v);
void file_location_free(FileLocation *loc);
bool file_location_equals(const FileLocation *a, const FileLocation *b);
bool file_location_go(const FileLocation *loc);
bool file_location_return(const FileLocation *loc);
void push_file_location(FileLocation *loc);
void pop_file_location(void);

#endif
