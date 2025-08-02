#ifndef COMPLETION_H
#define COMPLETION_H

#include "cmdline.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/macros.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

struct EditorState;

void complete_command_next(struct EditorState *e) NONNULL_ARGS;
void complete_command_prev(struct EditorState *e) NONNULL_ARGS;
void reset_completion(CommandLine *cmdline) NONNULL_ARGS;

// Like reset_completion(), but for use in contexts where there's
// nothing to reset most of the time
static inline void maybe_reset_completion(CommandLine *cmdline)
{
    CompletionState *cs = &cmdline->completion;
    if (likely(!cs->orig)) {
        BUG_ON(cs->parsed);
        BUG_ON(cs->completions.alloc != 0);
        return;
    }
    reset_completion(cmdline);
}

void collect_env(char **env, PointerArray *a, StringView prefix, const char *suffix) NONNULL_ARGS;
void collect_normal_aliases(struct EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_bound_normal_keys(struct EditorState *e, PointerArray *a, const char *keystr_prefix) NONNULL_ARGS;
void collect_hl_styles(struct EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_compilers(struct EditorState *e, PointerArray *a, const char *prefix) NONNULL_ARGS;
void collect_hashmap_keys(const HashMap *map, PointerArray *a, const char *prefix) NONNULL_ARGS;

#endif
