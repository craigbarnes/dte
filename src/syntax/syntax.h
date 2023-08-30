#ifndef SYNTAX_SYNTAX_H
#define SYNTAX_SYNTAX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "syntax/bitset.h"
#include "syntax/color.h"
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
    COND_INLIST_BUFFER,
    COND_RECOLOR,
    COND_RECOLOR_BUFFER,
    COND_STR,
    COND_STR2,
    COND_STR_ICASE,
    COND_HEREDOCEND,
} ConditionType;

typedef enum {
    STATE_INVALID = -1,
    STATE_EAT,
    STATE_NOEAT,
    STATE_NOEAT_BUFFER,
    STATE_HEREDOCBEGIN,
} DefaultActionType;

typedef struct {
    struct State *destination;

    // If condition has no emit name this is set to destination state's
    // emit name or list name (COND_INLIST)
    char *emit_name;

    // Set after all styles have been added (config loaded)
    const TermStyle *emit_style;
} Action;

typedef struct {
    HashSet strings;
    bool used;
    bool defined;
} StringList;

typedef union {
    BitSetWord bitset[BITSET_NR_WORDS(256)];
    StringView heredocend;
    const StringList *str_list;
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
    HashMap default_styles;
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

    DefaultActionType type;
    Action default_action;

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

static inline bool is_subsyntax(const Syntax *syn)
{
    return syn->name[0] == '.';
}

static inline bool cond_type_has_destination(ConditionType type)
{
    return !(type == COND_RECOLOR || type == COND_RECOLOR_BUFFER);
}

StringList *find_string_list(const Syntax *syn, const char *name);
State *find_state(const Syntax *syn, const char *name);
void finalize_syntax(HashMap *syntaxes, Syntax *syn, unsigned int saved_nr_errors);

Syntax *find_any_syntax(const HashMap *syntaxes, const char *name);
Syntax *find_syntax(const HashMap *syntaxes, const char *name);
void update_state_styles(const Syntax *syn, State *s, const StyleMap *styles);
void update_syntax_styles(Syntax *syn, const StyleMap *styles);
void update_all_syntax_styles(const HashMap *syntaxes, const StyleMap *styles);
void find_unused_subsyntaxes(const HashMap *syntaxes);
void free_syntaxes(HashMap *syntaxes);

#endif
