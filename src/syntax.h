#ifndef SYNTAX_H
#define SYNTAX_H

#include "libc.h"
#include "ptr-array.h"

enum condition_type {
	COND_BUFIS,
	COND_CHAR,
	COND_CHAR_BUFFER,
	COND_INLIST,
	COND_RECOLOR,
	COND_RECOLOR_BUFFER,
	COND_STR,
	COND_STR2,
	COND_STR_ICASE,
	COND_HEREDOCEND,
};

struct action {
	struct state *destination;

	// If condition has no emit name this is set to destination state's
	// emit name or list name (COND_LIST).
	char *emit_name;

	// Set after all colors have been added (config loaded).
	struct hl_color *emit_color;
};

struct condition {
	union {
		struct {
			int len;
			int icase;
			char str[256 / 8 - 2 * sizeof(int)];
		} cond_bufis;
		struct {
			unsigned char bitmap[256 / 8];
		} cond_char;
		struct {
			struct string_list *list;
		} cond_inlist;
		struct {
			int len;
		} cond_recolor;
		struct {
			int len;
			char str[256 / 8 - sizeof(int)];
		} cond_str;
		struct {
			int len;
			char *str;
		} cond_heredocend;
	} u;
	struct action a;
	enum condition_type type;
};

struct heredoc_state {
	struct state *state;
	char *delim;
	int len;
};

struct state {
	char *name;
	char *emit_name;
	struct ptr_array conds;

	bool defined;
	bool visited;
	bool copied;

	enum {
		STATE_EAT,
		STATE_NOEAT,
		STATE_NOEAT_BUFFER,
		STATE_HEREDOCBEGIN,
	} type;
	struct action a;

	struct {
		struct syntax *subsyntax;
		struct ptr_array states;
	} heredoc;
};

struct hash_str {
	struct hash_str *next;
	int len;
	char str[1];
};

struct string_list {
	char *name;
	struct hash_str *hash[62];
	bool icase;
	bool used;
	bool defined;
};

struct syntax {
	char *name;
	struct ptr_array states;
	struct ptr_array string_lists;
	struct ptr_array default_colors;
	bool heredoc;
	bool used;
};

struct syntax_merge {
	struct syntax *subsyn;
	struct state *return_state;
	const char *delim;
	int delim_len;
};

static inline bool is_subsyntax(struct syntax *syn)
{
	return syn->name[0] == '.';
}

unsigned int buf_hash(const char *str, unsigned int size);
struct string_list *find_string_list(struct syntax *syn, const char *name);
struct state *find_state(struct syntax *syn, const char *name);
struct state *merge_syntax(struct syntax *syn, struct syntax_merge *m);
void finalize_syntax(struct syntax *syn, int saved_nr_errors);

struct syntax *find_any_syntax(const char *name);
struct syntax *find_syntax(const char *name);
void update_syntax_colors(struct syntax *syn);
void update_all_syntax_colors(void);
void find_unused_subsyntaxes(void);

#endif
