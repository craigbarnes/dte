#ifndef TAG_H
#define TAG_H

#include <sys/types.h>
#include "ctags.h"
#include "msg.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

typedef struct {
    char *filename;
    char *buf;
    size_t size;
    time_t mtime;
} TagFile;

void add_message_for_tag(MessageArray *messages, Tag *tag, const StringView *dir) NONNULL_ARGS;
size_t tag_lookup(TagFile *tf, const StringView *name, const char *filename, MessageArray *messages) NONNULL_ARG(1, 2, 4);
void collect_tags(TagFile *tf, PointerArray *a, const StringView *prefix) NONNULL_ARGS;
void tag_file_free(TagFile *tf) NONNULL_ARGS;

#endif
