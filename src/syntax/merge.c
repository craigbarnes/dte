#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "merge.h"
#include "util/debug.h"
#include "util/hashmap.h"
#include "util/numtostr.h"
#include "util/string-view.h"
#include "util/xmalloc.h"
#include "util/xstring.h"

enum {
    FIXBUF_SIZE = 512
};

static const char *fix_name(char *buf, StringView prefix, const char *name)
{
    size_t plen = prefix.length;
    BUG_ON(plen >= FIXBUF_SIZE);
    size_t avail = FIXBUF_SIZE - plen;
    char *end = memccpy(xmempcpy(buf, prefix.data, plen), name, '\0', avail);
    FATAL_ERROR_ON(!end, ENOBUFS);
    return buf;
}

static void fix_action(const Syntax *syn, Action *a, StringView prefix, char *buf)
{
    if (a->destination) {
        const char *name = fix_name(buf, prefix, a->destination->name);
        a->destination = find_state(syn, name);
    }
}

static void fix_conditions (
    const Syntax *syn,
    State *s,
    const SyntaxMerge *m,
    StringView prefix,
    char *buf
) {
    BUG_ON(!s); // Silence spurious `-Wnull-dereference` warning in GCC 9
    for (size_t i = 0, n = s->conds.count; i < n; i++) {
        Condition *c = s->conds.ptrs[i];
        fix_action(syn, &c->a, prefix, buf);
        if (!c->a.destination && cond_type_has_destination(c->type)) {
            c->a.destination = m->return_state;
        }
        if (m->delim && c->type == COND_HEREDOCEND) {
            c->u.heredocend = string_view(m->delim, m->delim_len);
        }
    }

    fix_action(syn, &s->default_action, prefix, buf);
    if (!s->default_action.destination) {
        s->default_action.destination = m->return_state;
    }
}

// Generate a prefix for merged state names, to avoid clashes
static StringView make_prefix_str(char *buf)
{
    static unsigned int counter;
    size_t n = 0;
    buf[n++] = 'm';
    n += buf_uint_to_str(counter++, buf + n);
    buf[n++] = '-';
    return string_view(buf, n);
}

// Merge a sub-syntax into another syntax, copying or updating
// pointers and strings as appropriate.
// NOTE: string_lists is owned by Syntax, so there's no need to
// copy it. Freeing Condition does not free any string lists.
State *merge_syntax(Syntax *syn, SyntaxMerge *merge, const StyleMap *styles)
{
    const HashMap *subsyn_states = &merge->subsyn->states;
    HashMap *states = &syn->states;
    char prefix_buf[64];
    StringView prefix = make_prefix_str(prefix_buf);
    char buf[FIXBUF_SIZE];

    for (HashMapIter it = hashmap_iter(subsyn_states); hashmap_next(&it); ) {
        State *s = xmemdup(it.entry->value, sizeof(State));
        s->name = xmemjoin(prefix.data, prefix.length, s->name, strlen(s->name) + 1);
        hashmap_insert(states, s->name, s);

        if (s->conds.count > 0) {
            // Deep copy conds PointerArray
            BUG_ON(s->conds.alloc < s->conds.count);
            void **ptrs = xmallocarray(s->conds.alloc, sizeof(*ptrs));
            for (size_t i = 0, n = s->conds.count; i < n; i++) {
                ptrs[i] = xmemdup(s->conds.ptrs[i], sizeof(Condition));
            }
            s->conds.ptrs = ptrs;
        } else {
            BUG_ON(s->conds.alloc != 0);
        }

        // Mark unvisited, so that return-only states get visited
        s->visited = false;

        // Don't complain about unvisited, copied states
        s->copied = true;
    }

    // Fix conditions and update styles for newly merged states
    for (HashMapIter it = hashmap_iter(subsyn_states); hashmap_next(&it); ) {
        const State *subsyn_state = it.entry->value;
        BUG_ON(!subsyn_state);
        const char *new_name = fix_name(buf, prefix, subsyn_state->name);
        State *new_state = hashmap_xget(states, new_name);
        fix_conditions(syn, new_state, merge, prefix, buf);
        if (merge->delim) {
            update_state_styles(syn, new_state, styles);
        }
    }

    const char *name = fix_name(buf, prefix, merge->subsyn->start_state->name);
    State *start_state = hashmap_xget(states, name);
    merge->subsyn->used = true;
    return start_state;
}
