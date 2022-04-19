#ifndef TAG_H
#define TAG_H

#include "ctags.h"
#include "msg.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

void add_message_for_tag(MessageArray *messages, Tag *tag, const StringView *dir) NONNULL_ARGS;
void tag_lookup(const char *name, const char *filename, MessageArray *messages) NONNULL_ARG(1, 3);
void collect_tags(PointerArray *a, const char *prefix) NONNULL_ARGS;
void tag_file_free(void);

#endif
