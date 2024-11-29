#ifndef TAG_H
#define TAG_H

#include <sys/types.h>
#include "ctags.h"
#include "msg.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"
#include "util/string.h"

// Metadata and contents of a tags(5) file, as loaded by load_tag_file()
typedef struct {
    char *filename; // The absolute path of the tags file
    size_t dirname_len; // The length of the directory part of `filename` (including the last slash)
    char *buf; // The contents of the tags file
    size_t size; // The length of `buf`
    time_t mtime; // The modification time of the tags file (when last loaded)
} TagFile;

void add_message_for_tag(MessageArray *messages, Tag *tag, const StringView *dir) NONNULL_ARGS;
size_t tag_lookup(TagFile *tf, const StringView *name, const char *filename, MessageArray *messages) NONNULL_ARG(1, 2, 4);
void collect_tags(TagFile *tf, PointerArray *a, const StringView *prefix) NONNULL_ARGS;
String dump_tags(TagFile *tf) NONNULL_ARGS;
void tag_file_free(TagFile *tf) NONNULL_ARGS;

#endif
