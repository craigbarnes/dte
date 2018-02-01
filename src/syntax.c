#include "syntax.h"
#include "state.h"
#include "error.h"
#include "common.h"

static PointerArray syntaxes = PTR_ARRAY_INIT;

unsigned long buf_hash(const char *str, size_t size)
{
    unsigned long hash = 0;

    for (size_t i = 0; i < size; i++) {
        unsigned int ch = ascii_tolower(str[i]);
        hash = (hash << 5) - hash + ch;
    }
    return hash;
}

StringList *find_string_list(const Syntax *syn, const char *name)
{
    for (size_t i = 0; i < syn->string_lists.count; i++) {
        StringList *list = syn->string_lists.ptrs[i];
        if (streq(list->name, name)) {
            return list;
        }
    }
    return NULL;
}

State *find_state(const Syntax *syn, const char *name)
{
    for (size_t i = 0; i < syn->states.count; i++) {
        State *s = syn->states.ptrs[i];
        if (streq(s->name, name)) {
            return s;
        }
    }
    return NULL;
}

static bool has_destination(ConditionType type)
{
    switch (type) {
    case COND_RECOLOR:
    case COND_RECOLOR_BUFFER:
        return false;
    default:
        return true;
    }
}

Syntax *find_any_syntax(const char *name)
{
    for (size_t i = 0; i < syntaxes.count; i++) {
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

static void fix_action(Syntax *syn, Action *a, const char *prefix)
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
    Syntax *syn,
    State *s,
    SyntaxMerge *m,
    const char *prefix
) {
    for (size_t i = 0; i < s->conds.count; i++) {
        Condition *c = s->conds.ptrs[i];
        fix_action(syn, &c->a, prefix);
        if (c->a.destination == NULL && has_destination(c->type)) {
            c->a.destination = m->return_state;
        }

        if (m->delim && c->type == COND_HEREDOCEND) {
            c->u.cond_heredocend.str = xmemdup(m->delim, m->delim_len);
            c->u.cond_heredocend.len = m->delim_len;
        }
    }

    fix_action(syn, &s->a, prefix);
    if (s->a.destination == NULL) {
        s->a.destination = m->return_state;
    }
}

static const char *get_prefix(void)
{
    static int counter;
    static char prefix[32];
    snprintf(prefix, sizeof(prefix), "%d-", counter++);
    return prefix;
}

static void update_state_colors(Syntax *syn, State *s);

State *merge_syntax(Syntax *syn, SyntaxMerge *m)
{
    // NOTE: string_lists is owned by Syntax so there's no need to
    // copy it. Freeing Condition does not free any string lists.
    const char *prefix = get_prefix();
    PointerArray *states = &syn->states;
    size_t old_count = states->count;

    states->count += m->subsyn->states.count;
    if (states->count > states->alloc) {
        states->alloc = states->count;
        xrenew(states->ptrs, states->alloc);
    }
    memcpy (
        states->ptrs + old_count,
        m->subsyn->states.ptrs,
        sizeof(*states->ptrs) * m->subsyn->states.count
    );

    for (size_t i = old_count; i < states->count; i++) {
        State *s = xmemdup(states->ptrs[i], sizeof(State));

        states->ptrs[i] = s;
        s->name = xstrdup(fix_name(s->name, prefix));
        s->emit_name = xstrdup(s->emit_name);
        if (s->conds.count > 0) {
            s->conds.ptrs = xmemdup (
                s->conds.ptrs,
                sizeof(void *) * s->conds.alloc
            );
            for (size_t j = 0; j < s->conds.count; j++) {
                s->conds.ptrs[j] = xmemdup(s->conds.ptrs[j], sizeof(Condition));
            }
        }

        // Mark unvisited so that state that is used only as a return
        // state gets visited.
        s->visited = false;

        // Don't complain about unvisited copied states.
        s->copied = true;
    }

    for (size_t i = old_count; i < states->count; i++) {
        fix_conditions(syn, states->ptrs[i], m, prefix);
        if (m->delim) {
            update_state_colors(syn, states->ptrs[i]);
        }
    }

    m->subsyn->used = true;
    return states->ptrs[old_count];
}

static void visit(State *s)
{
    if (s->visited) {
        return;
    }
    s->visited = true;
    for (size_t i = 0; i < s->conds.count; i++) {
        Condition *cond = s->conds.ptrs[i];
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
    for (size_t i = 0; i < ARRAY_COUNT(list->hash); i++) {
        HashStr *h = list->hash[i];
        while (h) {
            HashStr *next = h->next;
            free(h);
            h = next;
        }
    }
    free(list->name);
    free(list);
}

static void free_syntax(Syntax *syn)
{
    ptr_array_free_cb(&syn->states, FREE_FUNC(free_state));
    ptr_array_free_cb(&syn->string_lists, FREE_FUNC(free_string_list));
    ptr_array_free_cb(&syn->default_colors, FREE_FUNC(free_strings));

    free(syn->name);
    free(syn);
}

void finalize_syntax(Syntax *syn, int saved_nr_errors)
{
    if (syn->states.count == 0) {
        error_msg("Empty syntax");
    }

    for (size_t i = 0; i < syn->states.count; i++) {
        State *s = syn->states.ptrs[i];
        if (!s->defined) {
            // This state has been referenced but not defined
            error_msg("No such state %s", s->name);
        }
    }
    for (size_t i = 0; i < syn->string_lists.count; i++) {
        StringList *list = syn->string_lists.ptrs[i];
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

    if (nr_errors != saved_nr_errors) {
        free_syntax(syn);
        return;
    }

    // Unused states and lists cause warning only
    visit(syn->states.ptrs[0]);
    for (size_t i = 0; i < syn->states.count; i++) {
        State *s = syn->states.ptrs[i];
        if (!s->visited && !s->copied) {
            error_msg("State %s is unreachable", s->name);
        }
    }
    for (size_t i = 0; i < syn->string_lists.count; i++) {
        StringList *list = syn->string_lists.ptrs[i];
        if (!list->used) {
            error_msg("List %s never used", list->name);
        }
    }

    ptr_array_add(&syntaxes, syn);
}

Syntax *find_syntax(const char *name)
{
    Syntax *syn = find_any_syntax(name);
    if (syn && is_subsyntax(syn)) {
        return NULL;
    }
    return syn;
}

static const char *find_default_color(Syntax *syn, const char *name)
{
    for (size_t i = 0; i < syn->default_colors.count; i++) {
        char **strs = syn->default_colors.ptrs[i];
        for (int j = 1; strs[j]; j++) {
            if (streq(strs[j], name)) {
                return strs[0];
            }
        }
    }
    return NULL;
}

static void update_action_color(Syntax *syn, Action *a)
{
    const char *name = a->emit_name;
    const char *def;
    char full[64];

    if (!name) {
        name = a->destination->emit_name;
    }

    snprintf(full, sizeof(full), "%s.%s", syn->name, name);
    a->emit_color = find_color(full);
    if (a->emit_color) {
        return;
    }

    def = find_default_color(syn, name);
    if (!def) {
        return;
    }

    snprintf(full, sizeof(full), "%s.%s", syn->name, def);
    a->emit_color = find_color(full);
}

static void update_state_colors(Syntax *syn, State *s)
{
    for (size_t i = 0; i < s->conds.count; i++) {
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
    for (size_t i = 0; i < syn->states.count; i++) {
        update_state_colors(syn, syn->states.ptrs[i]);
    }
}

void update_all_syntax_colors(void)
{
    for (size_t i = 0; i < syntaxes.count; i++) {
        update_syntax_colors(syntaxes.ptrs[i]);
    }
}

void find_unused_subsyntaxes(void)
{
    // Don't complain multiple times about same unused subsyntaxes
    static size_t i;

    for (; i < syntaxes.count; i++) {
        Syntax *s = syntaxes.ptrs[i];
        if (!s->used && is_subsyntax(s)) {
            error_msg("Subsyntax %s is unused", s->name);
        }
    }
}
