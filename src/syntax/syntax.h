#ifndef SYNTAX_SYNTAX_H
#define SYNTAX_SYNTAX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "bitset.h"
#include "color.h"
#include "util/hashmap.h"
#include "util/hashset.h"
#include "util/ptr-array.h"
#include "util/string-view.h"

typedef enum {
    COND_BUFIS,
    COND_BUFIS_ICASE,
    COND_CHAR,
    COND_CHAR_BUFFER,
    COND_CHAR1,
    COND_INLIST,
    COND_RECOLOR,
    COND_RECOLOR_BUFFER,
    COND_STR,
    COND_STR2,
    COND_STR_ICASE,
    COND_HEREDOCEND,
} ConditionType;

typedef struct {
    struct State *destination;

    // If condition has no emit name this is set to destination state's
    // emit name or list name (COND_INLIST).
    char *emit_name;

    // Set after all colors have been added (config loaded).
    TermColor *emit_color;
} Action;

typedef struct {
    HashSet strings;
    bool used;
    bool defined;
} StringList;

typedef union {
    BitSetWord bitset[BITSET_NR_WORDS(256)];
    StringView heredocend;
    StringList *str_list;
    unsigned char ch;
    size_t recolor_len;
    struct {
        uint8_t len;
        unsigned char buf[31];
    } str;
} ConditionData;

typedef struct {
    ConditionType type;
    ConditionData u;
    Action a;
} Condition;

typedef struct {
    char *name;
    HashMap states;
    struct State *start_state;
    HashMap string_lists;
    HashMap default_colors;
    bool heredoc;
    bool used;
    bool warned_unused_subsyntax;
} Syntax;

typedef struct State {
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

typedef struct {
    State *state;
    char *delim;
    size_t len;
} HeredocState;

typedef struct {
    Syntax *subsyn;
    State *return_state;
    const char *delim;
    size_t delim_len;
} SyntaxMerge;

static inline bool is_subsyntax(const Syntax *syn)
{
    return syn->name[0] == '.';
}

StringList *find_string_list(const Syntax *syn, const char *name);
State *find_state(const Syntax *syn, const char *name);
State *merge_syntax(Syntax *syn, SyntaxMerge *m);
void finalize_syntax(Syntax *syn, unsigned int saved_nr_errors);

Syntax *find_any_syntax(const char *name);
Syntax *find_syntax(const char *name);
void update_syntax_colors(Syntax *syn);
void update_all_syntax_colors(void);
void find_unused_subsyntaxes(void);
void free_syntaxes(void);

#endif
