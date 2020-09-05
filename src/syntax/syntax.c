#include <stdio.h>
#include "syntax.h"
#include "state.h"
#include "error.h"
#include "util/ascii.h"
#include "util/str-util.h"
#include "util/xmalloc.h"

static PointerArray syntaxes = PTR_ARRAY_INIT;

StringList *find_string_list(const Syntax *syn, const char *name)
{
    for (size_t i = 0, n = syn->string_lists.count; i < n; i++) {
        StringList *list = syn->string_lists.ptrs[i];
        if (streq(list->name, name)) {
            return list;
        }
    }
    return NULL;
}

State *find_state(const Syntax *syn, const char *name)
{
    for (size_t i = 0, n = syn->states.count; i < n; i++) {
        State *s = syn->states.ptrs[i];
        if (streq(s->name, name)) {
            return s;
        }
    }
    return NULL;
}

static bool has_destination(ConditionType type)
{
    return !(type == COND_RECOLOR || type == COND_RECOLOR_BUFFER);
}

Syntax *find_any_syntax(const char *name)
{
    for (size_t i = 0, n = syntaxes.count; i < n; i++) {
        Syntax *syn = syntaxes.ptrs[i];
        if (streq(syn->name, name)) {
            return syn;
        }
    }
    return NULL;
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
    PointerArray *states = &syn->states;
    size_t old_count = states->count;

    states->count += merge->subsyn->states.count;
    if (states->count > states->alloc) {
        states->alloc = states->count;
        xrenew(states->ptrs, states->alloc);
    }
    memcpy (
        states->ptrs + old_count,
        merge->subsyn->states.ptrs,
        sizeof(*states->ptrs) * merge->subsyn->states.count
    );

    for (size_t i = old_count, n = states->count; i < n; i++) {
        State *s = xmemdup(states->ptrs[i], sizeof(State));
        states->ptrs[i] = s;
        s->name = xstrdup(fix_name(s->name, prefix));
        s->emit_name = xstrdup(s->emit_name);
        if (s->conds.count > 0) {
            s->conds.ptrs = xmemdup (
                s->conds.ptrs,
                sizeof(void *) * s->conds.alloc
            );
            for (size_t j = 0, n2 = s->conds.count; j < n2; j++) {
                s->conds.ptrs[j] = xmemdup(s->conds.ptrs[j], sizeof(Condition));
            }
        }

        // Mark unvisited so that state that is used only as a return
        // state gets visited.
        s->visited = false;

        // Don't complain about unvisited copied states.
        s->copied = true;
    }

    for (size_t i = old_count, n = states->count; i < n; i++) {
        fix_conditions(syn, states->ptrs[i], merge, prefix);
        if (merge->delim) {
            update_state_colors(syn, states->ptrs[i]);
        }
    }

    merge->subsyn->used = true;
    return states->ptrs[old_count];
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
    free(s->name);
    free(s->emit_name);
    ptr_array_free_cb(&s->conds, FREE_FUNC(free_condition));
    free(s->a.emit_name);
    free(s);
}

static void free_string_list(StringList *list)
{
    hashset_free(&list->strings);
    free(list->name);
    free(list);
}

static void free_syntax(Syntax *syn)
{
    ptr_array_free_cb(&syn->states, FREE_FUNC(free_state));
    ptr_array_free_cb(&syn->string_lists, FREE_FUNC(free_string_list));
    ptr_array_free_cb(&syn->default_colors, FREE_FUNC(free_string_array));

    free(syn->name);
    free(syn);
}

void finalize_syntax(Syntax *syn, unsigned int saved_nr_errors)
{
    if (syn->states.count == 0) {
        error_msg("Empty syntax");
    }

    for (size_t i = 0, n = syn->states.count; i < n; i++) {
        const State *s = syn->states.ptrs[i];
        if (!s->defined) {
            // This state has been referenced but not defined
            error_msg("No such state %s", s->name);
        }
    }
    for (size_t i = 0, n = syn->string_lists.count; i < n; i++) {
        const StringList *list = syn->string_lists.ptrs[i];
        if (!list->defined) {
            error_msg("No such list %s", list->name);
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
    visit(syn->states.ptrs[0]);
    for (size_t i = 0, n = syn->states.count; i < n; i++) {
        const State *s = syn->states.ptrs[i];
        if (!s->visited && !s->copied) {
            error_msg("State %s is unreachable", s->name);
        }
    }
    for (size_t i = 0, n = syn->string_lists.count; i < n; i++) {
        const StringList *list = syn->string_lists.ptrs[i];
        if (!list->used) {
            error_msg("List %s never used", list->name);
        }
    }

    ptr_array_append(&syntaxes, syn);
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
    for (size_t i = 0, n = syn->default_colors.count; i < n; i++) {
        char **strs = syn->default_colors.ptrs[i];
        for (size_t j = 1; strs[j]; j++) {
            if (streq(strs[j], name)) {
                return strs[0];
            }
        }
    }
    return NULL;
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
    for (size_t i = 0, n = syn->states.count; i < n; i++) {
        update_state_colors(syn, syn->states.ptrs[i]);
    }
}

void update_all_syntax_colors(void)
{
    for (size_t i = 0, n = syntaxes.count; i < n; i++) {
        update_syntax_colors(syntaxes.ptrs[i]);
    }
}

void find_unused_subsyntaxes(void)
{
    // Don't complain multiple times about same unused subsyntaxes
    static size_t i;

    for (size_t n = syntaxes.count; i < n; i++) {
        const Syntax *s = syntaxes.ptrs[i];
        if (!s->used && is_subsyntax(s)) {
            error_msg("Subsyntax %s is unused", s->name);
        }
    }
}
