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

typedef struct action {
    struct state *destination;

    // If condition has no emit name this is set to destination state's
    // emit name or list name (COND_LIST).
    char *emit_name;

    // Set after all colors have been added (config loaded).
    struct hl_color *emit_color;
} Action;

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
    Action a;
    enum condition_type type;
};

struct heredoc_state {
    struct state *state;
    char *delim;
    int len;
};

typedef struct syntax {
    char *name;
    PointerArray states;
    PointerArray string_lists;
    PointerArray default_colors;
    bool heredoc;
    bool used;
} Syntax;

typedef struct state {
    char *name;
    char *emit_name;
    PointerArray conds;

    bool defined;
    bool visited;
    bool copied;

    enum {
        STATE_INVALID = -1,
        STATE_EAT,
        STATE_NOEAT,
        STATE_NOEAT_BUFFER,
        STATE_HEREDOCBEGIN,
    } type;
    Action a;

    struct {
        Syntax *subsyntax;
        PointerArray states;
    } heredoc;
} State;

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

typedef struct {
    Syntax *subsyn;
    State *return_state;
    const char *delim;
    int delim_len;
} SyntaxMerge;

static inline bool is_subsyntax(Syntax *syn)
{
    return syn->name[0] == '.';
}

unsigned long buf_hash(const char *str, size_t size);
struct string_list *find_string_list(Syntax *syn, const char *name);
State *find_state(Syntax *syn, const char *name);
State *merge_syntax(Syntax *syn, SyntaxMerge *m);
void finalize_syntax(Syntax *syn, int saved_nr_errors);

Syntax *find_any_syntax(const char *name);
Syntax *find_syntax(const char *name);
void update_syntax_colors(Syntax *syn);
void update_all_syntax_colors(void);
void find_unused_subsyntaxes(void);

#endif
