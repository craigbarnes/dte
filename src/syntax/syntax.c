#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "syntax.h"
#include "error.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static HashMap syntaxes = HASHMAP_INIT;

StringList *find_string_list(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->string_lists, name);
}

State *find_state(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->states, name);
}

static bool has_destination(ConditionType type)
{
    return !(type == COND_RECOLOR || type == COND_RECOLOR_BUFFER);
}

Syntax *find_any_syntax(const char *name)
{
    return hashmap_get(&syntaxes, name);
}

static const char *fix_name(const char *name, const char *prefix)
{
    static char buf[64];
    snprintf(buf, sizeof(buf), "%s%s", prefix, name);
    return buf;
}

static void fix_action(const Syntax *syn, Action *a, const char *prefix)
{
    if (a->destination) {
        const char *name = fix_name(a->destination->name, prefix);
        a->destination = find_state(syn, name);
    }
    if (a->emit_name) {
        a->emit_name = xstrdup(a->emit_name);
    }
}

static void fix_conditions (
    const Syntax *syn,
    State *s,
    const SyntaxMerge *m,
    const char *prefix
) {
    for (size_t i = 0, n = s->conds.count; i < n; i++) {
        Condition *c = s->conds.ptrs[i];
        fix_action(syn, &c->a, prefix);
        if (!c->a.destination && has_destination(c->type)) {
            c->a.destination = m->return_state;
        }

        if (m->delim && c->type == COND_HEREDOCEND) {
            c->u.heredocend.data = xmemdup(m->delim, m->delim_len);
            c->u.heredocend.length = m->delim_len;
        }
    }

    fix_action(syn, &s->a, prefix);
    if (!s->a.destination) {
        s->a.destination = m->return_state;
    }
}

static const char *get_prefix(void)
{
    static unsigned int counter;
    static char prefix[32];
    snprintf(prefix, sizeof(prefix), "%u-", counter++);
    return prefix;
}

static void update_state_colors(const Syntax *syn, State *s);

State *merge_syntax(Syntax *syn, SyntaxMerge *merge)
{
    // NOTE: string_lists is owned by Syntax so there's no need to
    // copy it. Freeing Condition does not free any string lists.
    const char *prefix = get_prefix();
    const HashMap *subsyn_states = &merge->subsyn->states;
    HashMap *states = &syn->states;

    for (HashMapIter it = hashmap_iter(subsyn_states); hashmap_next(&it); ) {
        State *s = xmemdup(it.entry->value, sizeof(State));
        s->name = xstrdup(fix_name(s->name, prefix));
        s->emit_name = xstrdup(s->emit_name);
        hashmap_insert(states, s->name, s);

        if (s->conds.count > 0) {
            // Deep copy conds PointerArray
            BUG_ON(s->conds.alloc < s->conds.count);
            void **ptrs = xnew(void*, s->conds.alloc);
            for (size_t i = 0, n = s->conds.count; i < n; i++) {
                ptrs[i] = xmemdup(s->conds.ptrs[i], sizeof(Condition));
            }
            s->conds.ptrs = ptrs;
        } else {
            BUG_ON(s->conds.alloc != 0);
        }

        // Mark unvisited so that state that is used only as a return
        // state gets visited.
        s->visited = false;

        // Don't complain about unvisited copied states.
        s->copied = true;
    }

    // Fix conditions and update colors for newly merged states
    for (HashMapIter it = hashmap_iter(subsyn_states); hashmap_next(&it); ) {
        const State *subsyn_state = it.entry->value;
        BUG_ON(!subsyn_state);
        const char *new_name = fix_name(subsyn_state->name, prefix);
        State *new_state = hashmap_get(states, new_name);
        BUG_ON(!new_state);
        fix_conditions(syn, new_state, merge, prefix);
        if (merge->delim) {
            update_state_colors(syn, new_state);
        }
    }

    const char *name = fix_name(merge->subsyn->start_state->name, prefix);
    State *start_state = hashmap_get(states, name);
    BUG_ON(!start_state);
    merge->subsyn->used = true;
    return start_state;
}

static void visit(State *s)
{
    if (s->visited) {
        return;
    }
    s->visited = true;
    for (size_t i = 0, n = s->conds.count; i < n; i++) {
        const Condition *cond = s->conds.ptrs[i];
        if (cond->a.destination) {
            visit(cond->a.destination);
        }
    }
    if (s->a.destination) {
        visit(s->a.destination);
    }
}

static void free_condition(Condition *cond)
{
    free(cond->a.emit_name);
    free(cond);
}

static void free_state(State *s)
{
    free(s->emit_name);
    ptr_array_free_cb(&s->conds, FREE_FUNC(free_condition));
    free(s->a.emit_name);
    free(s);
}

static void free_string_list(StringList *list)
{
    hashset_free(&list->strings);
    free(list);
}

static void free_syntax_contents(Syntax *syn)
{
    hashmap_free(&syn->states, FREE_FUNC(free_state));
    hashmap_free(&syn->string_lists, FREE_FUNC(free_string_list));
    hashmap_free(&syn->default_colors, NULL);
}

static void free_syntax(Syntax *syn)
{
    free_syntax_contents(syn);
    free(syn->name);
    free(syn);
}

static void free_syntax_cb(Syntax *syn)
{
    free_syntax_contents(syn);
    free(syn);
}

// This function is only called by the test binary, just to ensure
// the various free_*() functions get exercised by ASan/UBSan
void free_syntaxes(void)
{
    hashmap_free(&syntaxes, FREE_FUNC(free_syntax_cb));
}

void finalize_syntax(Syntax *syn, unsigned int saved_nr_errors)
{
    if (syn->states.count == 0) {
        error_msg("Empty syntax");
    }

    for (HashMapIter it = hashmap_iter(&syn->states); hashmap_next(&it); ) {
        const State *s = it.entry->value;
        if (!s->defined) {
            // This state has been referenced but not defined
            error_msg("No such state %s", it.entry->key);
        }
    }
    for (HashMapIter it = hashmap_iter(&syn->string_lists); hashmap_next(&it); ) {
        const StringList *list = it.entry->value;
        if (!list->defined) {
            error_msg("No such list %s", it.entry->key);
        }
    }

    if (syn->heredoc && !is_subsyntax(syn)) {
        error_msg("heredocend can be used only in subsyntaxes");
    }

    if (find_any_syntax(syn->name)) {
        error_msg("Syntax %s already exists", syn->name);
    }

    if (get_nr_errors() != saved_nr_errors) {
        free_syntax(syn);
        return;
    }

    // Unused states and lists cause warning only
    visit(syn->start_state);
    for (HashMapIter it = hashmap_iter(&syn->states); hashmap_next(&it); ) {
        const State *s = it.entry->value;
        if (!s->visited && !s->copied) {
            error_msg("State %s is unreachable", it.entry->key);
        }
    }
    for (HashMapIter it = hashmap_iter(&syn->string_lists); hashmap_next(&it); ) {
        const StringList *list = it.entry->value;
        if (!list->used) {
            error_msg("List %s never used", it.entry->key);
        }
    }

    hashmap_insert(&syntaxes, syn->name, syn);
}

Syntax *find_syntax(const char *name)
{
    Syntax *syn = find_any_syntax(name);
    if (syn && is_subsyntax(syn)) {
        return NULL;
    }
    return syn;
}

static const char *find_default_color(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->default_colors, name);
}

static void update_action_color(const Syntax *syn, Action *a)
{
    const char *name = a->emit_name;
    if (!name) {
        name = a->destination->emit_name;
    }

    char full[64];
    snprintf(full, sizeof(full), "%s.%s", syn->name, name);
    a->emit_color = find_color(full);
    if (a->emit_color) {
        return;
    }

    const char *def = find_default_color(syn, name);
    if (!def) {
        return;
    }

    snprintf(full, sizeof(full), "%s.%s", syn->name, def);
    a->emit_color = find_color(full);
}

static void update_state_colors(const Syntax *syn, State *s)
{
    for (size_t i = 0, n = s->conds.count; i < n; i++) {
        Condition *c = s->conds.ptrs[i];
        update_action_color(syn, &c->a);
    }
    update_action_color(syn, &s->a);
}

void update_syntax_colors(Syntax *syn)
{
    if (is_subsyntax(syn)) {
        // No point to update colors of a sub-syntax
        return;
    }
    for (HashMapIter it = hashmap_iter(&syn->states); hashmap_next(&it); ) {
        update_state_colors(syn, it.entry->value);
    }
}

void update_all_syntax_colors(void)
{
    for (HashMapIter it = hashmap_iter(&syntaxes); hashmap_next(&it); ) {
        update_syntax_colors(it.entry->value);
    }
}

void find_unused_subsyntaxes(void)
{
    for (HashMapIter it = hashmap_iter(&syntaxes); hashmap_next(&it); ) {
        Syntax *s = it.entry->value;
        if (!s->used && !s->warned_unused_subsyntax && is_subsyntax(s)) {
            error_msg("Subsyntax %s is unused", s->name);
            // Don't complain multiple times about the same unused subsyntaxes
            s->warned_unused_subsyntax = true;
        }
    }
}
