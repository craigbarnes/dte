#include <stdlib.h>
#include "syntax.h"
#include "error.h"
#include "util/xsnprintf.h"

StringList *find_string_list(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->string_lists, name);
}

State *find_state(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->states, name);
}

Syntax *find_any_syntax(const HashMap *syntaxes, const char *name)
{
    return hashmap_get(syntaxes, name);
}

// NOLINTNEXTLINE(misc-no-recursion)
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
    if (s->default_action.destination) {
        visit(s->default_action.destination);
    }
}

static void free_condition(Condition *cond)
{
    free(cond->a.emit_name);
    free(cond);
}

static void free_heredoc_state(HeredocState *s)
{
    free(s);
}

static void free_state(State *s)
{
    free(s->emit_name);
    ptr_array_free_cb(&s->conds, FREE_FUNC(free_condition));
    ptr_array_free_cb(&s->heredoc.states, FREE_FUNC(free_heredoc_state));
    free(s->default_action.emit_name);
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
    hashmap_free(&syn->default_styles, NULL);
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

void free_syntaxes(HashMap *syntaxes)
{
    hashmap_free(syntaxes, FREE_FUNC(free_syntax_cb));
}

void finalize_syntax(HashMap *syntaxes, Syntax *syn, unsigned int saved_nr_errors)
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

    if (find_any_syntax(syntaxes, syn->name)) {
        error_msg("Syntax %s already exists", syn->name);
    }

    if (get_nr_errors() != saved_nr_errors) {
        free_syntax(syn);
        return;
    }

    // Unused states and lists cause warnings only (to make loading WIP
    // syntax files less annoying)
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

    hashmap_insert(syntaxes, syn->name, syn);
}

Syntax *find_syntax(const HashMap *syntaxes, const char *name)
{
    Syntax *syn = find_any_syntax(syntaxes, name);
    if (syn && is_subsyntax(syn)) {
        return NULL;
    }
    return syn;
}

static const char *find_default_style(const Syntax *syn, const char *name)
{
    return hashmap_get(&syn->default_styles, name);
}

static void update_action_style(const Syntax *syn, Action *a, const StyleMap *styles)
{
    const char *name = a->emit_name;
    if (!name) {
        name = a->destination->emit_name;
    }

    char full[256];
    xsnprintf(full, sizeof full, "%s.%s", syn->name, name);
    a->emit_style = find_style(styles, full);
    if (a->emit_style) {
        return;
    }

    const char *def = find_default_style(syn, name);
    if (!def) {
        return;
    }

    xsnprintf(full, sizeof full, "%s.%s", syn->name, def);
    a->emit_style = find_style(styles, full);
}

void update_state_styles(const Syntax *syn, State *s, const StyleMap *styles)
{
    for (size_t i = 0, n = s->conds.count; i < n; i++) {
        Condition *c = s->conds.ptrs[i];
        update_action_style(syn, &c->a, styles);
    }
    update_action_style(syn, &s->default_action, styles);
}

void update_syntax_styles(Syntax *syn, const StyleMap *styles)
{
    if (is_subsyntax(syn)) {
        // No point in updating styles of a sub-syntax
        return;
    }
    for (HashMapIter it = hashmap_iter(&syn->states); hashmap_next(&it); ) {
        update_state_styles(syn, it.entry->value, styles);
    }
}

void update_all_syntax_styles(const HashMap *syntaxes, const StyleMap *styles)
{
    for (HashMapIter it = hashmap_iter(syntaxes); hashmap_next(&it); ) {
        update_syntax_styles(it.entry->value, styles);
    }
}

void find_unused_subsyntaxes(const HashMap *syntaxes)
{
    for (HashMapIter it = hashmap_iter(syntaxes); hashmap_next(&it); ) {
        Syntax *s = it.entry->value;
        if (!s->used && !s->warned_unused_subsyntax && is_subsyntax(s)) {
            error_msg("Subsyntax %s is unused", s->name);
            // Don't complain multiple times about the same unused subsyntaxes
            s->warned_unused_subsyntax = true;
        }
    }
}
