#include "syntax.h"
#include "state.h"
#include "color.h"
#include "ptr-array.h"
#include "error.h"
#include "common.h"

static PTR_ARRAY(syntaxes);

unsigned int buf_hash(const char *str, unsigned int size)
{
	long i, hash = 0;

	for (i = 0; i < size; i++) {
		unsigned int ch = tolower(str[i]);
		hash = (hash << 5) - hash + ch;
	}
	return hash;
}

struct string_list *find_string_list(struct syntax *syn, const char *name)
{
	int i;

	for (i = 0; i < syn->string_lists.count; i++) {
		struct string_list *list = syn->string_lists.ptrs[i];
		if (streq(list->name, name))
			return list;
	}
	return NULL;
}

struct state *find_state(struct syntax *syn, const char *name)
{
	int i;

	for (i = 0; i < syn->states.count; i++) {
		struct state *s = syn->states.ptrs[i];
		if (streq(s->name, name))
			return s;
	}
	return NULL;
}

static bool has_destination(enum condition_type type)
{
	switch (type) {
	case COND_RECOLOR:
	case COND_RECOLOR_BUFFER:
		return false;
	default:
		return true;
	}
}

struct syntax *find_any_syntax(const char *name)
{
	int i;

	for (i = 0; i < syntaxes.count; i++) {
		struct syntax *syn = syntaxes.ptrs[i];
		if (streq(syn->name, name))
			return syn;
	}
	return NULL;
}

static const char *fix_name(const char *name, const char *prefix)
{
	static char buf[64];
	snprintf(buf, sizeof(buf), "%s%s", prefix, name);
	return buf;
}

static void fix_action(struct syntax *syn, struct action *a, const char *prefix)
{
	if (a->destination) {
		const char *name = fix_name(a->destination->name, prefix);
		a->destination = find_state(syn, name);
	}
	if (a->emit_name)
		a->emit_name = xstrdup(a->emit_name);
}

static void fix_conditions(struct syntax *syn, struct state *s, struct syntax_merge *m, const char *prefix)
{
	int i;

	for (i = 0; i < s->conds.count; i++) {
		struct condition *c = s->conds.ptrs[i];
		fix_action(syn, &c->a, prefix);
		if (c->a.destination == NULL && has_destination(c->type))
			c->a.destination = m->return_state;

		if (m->delim && c->type == COND_HEREDOCEND) {
			c->u.cond_heredocend.str = xmemdup(m->delim, m->delim_len);
			c->u.cond_heredocend.len = m->delim_len;
		}
	}

	fix_action(syn, &s->a, prefix);
	if (s->a.destination == NULL)
		s->a.destination = m->return_state;
}

static const char *get_prefix(void)
{
	static int counter;
	static char prefix[32];
	snprintf(prefix, sizeof(prefix), "%d-", counter++);
	return prefix;
}

static void update_state_colors(struct syntax *syn, struct state *s);

struct state *merge_syntax(struct syntax *syn, struct syntax_merge *m)
{
	// NOTE: string_lists is owned by struct syntax so there's no need to
	// copy it.  Freeing struct condition does not free any string lists.
	const char *prefix = get_prefix();
	struct ptr_array *states = &syn->states;
	int i, old_count = states->count;

	states->count += m->subsyn->states.count;
	if (states->count > states->alloc) {
		states->alloc = states->count;
		xrenew(states->ptrs, states->alloc);
	}
	memcpy(states->ptrs + old_count, m->subsyn->states.ptrs, sizeof(*states->ptrs) * m->subsyn->states.count);

	for (i = old_count; i < states->count; i++) {
		struct state *s = xmemdup(states->ptrs[i], sizeof(struct state));
		int j;

		states->ptrs[i] = s;
		s->name = xstrdup(fix_name(s->name, prefix));
		s->emit_name = xstrdup(s->emit_name);
		s->conds.ptrs = xmemdup(s->conds.ptrs, sizeof(void *) * s->conds.alloc);
		for (j = 0; j < s->conds.count; j++)
			s->conds.ptrs[j] = xmemdup(s->conds.ptrs[j], sizeof(struct condition));

		// Mark unvisited so that state that is used only as a return state gets visited.
		s->visited = false;

		// Don't complain about unvisited copied states.
		s->copied = true;
	}

	for (i = old_count; i < states->count; i++) {
		fix_conditions(syn, states->ptrs[i], m, prefix);
		if (m->delim)
			update_state_colors(syn, states->ptrs[i]);
	}

	m->subsyn->used = true;
	return states->ptrs[old_count];
}

static void visit(struct state *s)
{
	int i;

	if (s->visited)
		return;

	s->visited = true;
	for (i = 0; i < s->conds.count; i++) {
		struct condition *cond = s->conds.ptrs[i];
		if (cond->a.destination)
			visit(cond->a.destination);
	}
	if (s->a.destination)
		visit(s->a.destination);
}

static void free_condition(struct condition *cond)
{
	free(cond->a.emit_name);
	free(cond);
}

static void free_state(struct state *s)
{
	free(s->name);
	free(s->emit_name);
	ptr_array_free_cb(&s->conds, FREE_FUNC(free_condition));
	free(s->a.emit_name);
	free(s);
}

static void free_string_list(struct string_list *list)
{
	int i;

	for (i = 0; i < ARRAY_COUNT(list->hash); i++) {
		struct hash_str *h = list->hash[i];
		while (h) {
			struct hash_str *next = h->next;
			free(h);
			h = next;
		}
	}
	free(list->name);
	free(list);
}

static void free_syntax(struct syntax *syn)
{
	ptr_array_free_cb(&syn->states, FREE_FUNC(free_state));
	ptr_array_free_cb(&syn->string_lists, FREE_FUNC(free_string_list));
	ptr_array_free_cb(&syn->default_colors, FREE_FUNC(free_strings));

	free(syn->name);
	free(syn);
}

void finalize_syntax(struct syntax *syn, int saved_nr_errors)
{
	int i;

	if (syn->states.count == 0)
		error_msg("Empty syntax");

	for (i = 0; i < syn->states.count; i++) {
		struct state *s = syn->states.ptrs[i];
		if (!s->defined) {
			// this state has been referenced but not defined
			error_msg("No such state %s", s->name);
		}
	}
	for (i = 0; i < syn->string_lists.count; i++) {
		struct string_list *list = syn->string_lists.ptrs[i];
		if (!list->defined)
			error_msg("No such list %s", list->name);
	}

	if (syn->heredoc && !is_subsyntax(syn))
		error_msg("heredocend can be used only in subsyntaxes");

	if (find_any_syntax(syn->name))
		error_msg("Syntax %s already exists", syn->name);

	if (nr_errors != saved_nr_errors) {
		free_syntax(syn);
		return;
	}

	// unused states and lists cause warning only
	visit(syn->states.ptrs[0]);
	for (i = 0; i < syn->states.count; i++) {
		struct state *s = syn->states.ptrs[i];
		if (!s->visited && !s->copied)
			error_msg("State %s is unreachable", s->name);
	}
	for (i = 0; i < syn->string_lists.count; i++) {
		struct string_list *list = syn->string_lists.ptrs[i];
		if (!list->used)
			error_msg("List %s never used", list->name);
	}

	ptr_array_add(&syntaxes, syn);
}

struct syntax *find_syntax(const char *name)
{
	struct syntax *syn = find_any_syntax(name);
	if (syn && is_subsyntax(syn))
		return NULL;
	return syn;
}

static const char *find_default_color(struct syntax *syn, const char *name)
{
	int i, j;

	for (i = 0; i < syn->default_colors.count; i++) {
		char **strs = syn->default_colors.ptrs[i];
		for (j = 1; strs[j]; j++) {
			if (streq(strs[j], name))
				return strs[0];
		}
	}
	return NULL;
}

static void update_action_color(struct syntax *syn, struct action *a)
{
	const char *name = a->emit_name;
	const char *def;
	char full[64];

	if (!name)
		name = a->destination->emit_name;

	snprintf(full, sizeof(full), "%s.%s", syn->name, name);
	a->emit_color = find_color(full);
	if (a->emit_color)
		return;

	def = find_default_color(syn, name);
	if (!def)
		return;

	snprintf(full, sizeof(full), "%s.%s", syn->name, def);
	a->emit_color = find_color(full);
}

static void update_state_colors(struct syntax *syn, struct state *s)
{
	int i;

	for (i = 0; i < s->conds.count; i++) {
		struct condition *c = s->conds.ptrs[i];
		update_action_color(syn, &c->a);
	}
	update_action_color(syn, &s->a);
}

void update_syntax_colors(struct syntax *syn)
{
	int i;

	if (is_subsyntax(syn)) {
		// no point to update colors of a sub-syntax
		return;
	}
	for (i = 0; i < syn->states.count; i++)
		update_state_colors(syn, syn->states.ptrs[i]);
}

void update_all_syntax_colors(void)
{
	int i;

	for (i = 0; i < syntaxes.count; i++)
		update_syntax_colors(syntaxes.ptrs[i]);
}

void find_unused_subsyntaxes(void)
{
	// don't complain multiple times about same unused subsyntaxes
	static int i;

	for (; i < syntaxes.count; i++) {
		struct syntax *s = syntaxes.ptrs[i];
		if (!s->used && is_subsyntax(s))
			error_msg("Subsyntax %s is unused", s->name);
	}
}
