#ifndef SYNTAX_H
#define SYNTAX_H

#include <stdbool.h>
#include <stddef.h>
#include "color.h"
#include "ptr-array.h"

typedef enum {
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
} ConditionType;

typedef struct State State;
typedef struct HashStr HashStr;

typedef struct {
    State *destination;

    // If condition has no emit name this is set to destination state's
    // emit name or list name (COND_LIST).
    char *emit_name;

    // Set after all colors have been added (config loaded).
    HlColor *emit_color;
} Action;

struct HashStr {
    HashStr *next;
    size_t len;
    char str[1];
};

typedef struct {
    char *name;
    HashStr *hash[62];
    bool icase;
    bool used;
    bool defined;
} StringList;

typedef struct {
    union {
        struct {
            size_t len;
            bool icase;
            char str[256 / 8 - 2 * sizeof(int)];
        } cond_bufis;
        struct {
            unsigned char bitmap[256 / 8];
        } cond_char;
        struct {
            StringList *list;
        } cond_inlist;
        struct {
            size_t len;
        } cond_recolor;
        struct {
            size_t len;
            char str[256 / 8 - sizeof(int)];
        } cond_str;
        struct {
            size_t len;
            char *str;
        } cond_heredocend;
    } u;
    Action a;
    ConditionType type;
} Condition;

typedef struct {
    State *state;
    char *delim;
    size_t len;
} HeredocState;

typedef struct {
    char *name;
    PointerArray states;
    PointerArray string_lists;
    PointerArray default_colors;
    bool heredoc;
    bool used;
} Syntax;

struct State {
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
};

typedef struct {
    Syntax *subsyn;
    State *return_state;
    const char *delim;
    size_t delim_len;
} SyntaxMerge;

static inline bool is_subsyntax(Syntax *syn)
{
    return syn->name[0] == '.';
}

unsigned long buf_hash(const char *str, size_t size);
StringList *find_string_list(const Syntax *syn, const char *name);
State *find_state(const Syntax *syn, const char *name);
State *merge_syntax(Syntax *syn, SyntaxMerge *m);
void finalize_syntax(Syntax *syn, int saved_nr_errors);

Syntax *find_any_syntax(const char *name);
Syntax *find_syntax(const char *name);
void update_syntax_colors(Syntax *syn);
void update_all_syntax_colors(void);
void find_unused_subsyntaxes(void);

#endif
