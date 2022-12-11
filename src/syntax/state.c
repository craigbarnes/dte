#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "state.h"
#include "command/args.h"
#include "command/run.h"
#include "error.h"
#include "filetype.h"
#include "syntax/merge.h"
#include "util/bsearch.h"
#include "util/intern.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

typedef struct {
    const StringView *home_dir;
    const char *user_config_dir;
    const ColorScheme *colors;
    HashMap *syntaxes;
    Syntax *current_syntax;
    State *current_state;
    unsigned int saved_nr_errors; // Used to check if nr_errors changed
} SyntaxParser;

static bool no_syntax(const SyntaxParser *sp)
{
    if (likely(sp->current_syntax)) {
        return false;
    }
    error_msg("No syntax started");
    return true;
}

static bool no_state(const SyntaxParser *sp)
{
    if (unlikely(no_syntax(sp))) {
        return true;
    }
    if (likely(sp->current_state)) {
        return false;
    }
    error_msg("No state started");
    return true;
}

static void close_state(SyntaxParser *sp)
{
    if (!sp->current_state) {
        return;
    }
    if (unlikely(sp->current_state->type == STATE_INVALID)) {
        // Command prefix in error message makes no sense
        const Command *save = current_command;
        current_command = NULL;
        error_msg("No default action in state '%s'", sp->current_state->name);
        current_command = save;
    }
    sp->current_state = NULL;
}

static State *find_or_add_state(SyntaxParser *sp, const char *name)
{
    State *state = find_state(sp->current_syntax, name);
    if (state) {
        return state;
    }

    state = xnew0(State, 1);
    state->name = xstrdup(name);
    state->defined = false;
    state->type = STATE_INVALID;

    if (sp->current_syntax->states.count == 0) {
        sp->current_syntax->start_state = state;
    }

    return hashmap_insert(&sp->current_syntax->states, state->name, state);
}

static State *reference_state(SyntaxParser *sp, const char *name)
{
    if (streq(name, "this")) {
        return sp->current_state;
    }

    State *state = find_or_add_state(sp, name);
    if (unlikely(state == sp->current_state)) {
        LOG_WARNING (
            "destination '%s' can be optimized to 'this' in '%s' syntax",
            name,
            sp->current_syntax->name
        );
    }
    return state;
}

static bool not_subsyntax(const SyntaxParser *sp)
{
    if (likely(is_subsyntax(sp->current_syntax))) {
        return false;
    }
    error_msg("Destination state 'END' only allowed in a subsyntax");
    return true;
}

static Syntax *must_find_subsyntax(SyntaxParser *sp, const char *name)
{
    Syntax *syntax = find_any_syntax(sp->syntaxes, name);
    if (unlikely(!syntax)) {
        error_msg("No such syntax '%s'", name);
        return NULL;
    }
    if (unlikely(!is_subsyntax(syntax))) {
        error_msg("Syntax '%s' is not a subsyntax", name);
        return NULL;
    }
    return syntax;
}

static bool subsyntax_call(SyntaxParser *sp, const char *name, const char *ret, State **dest)
{
    Syntax *subsyn = must_find_subsyntax(sp, name);

    SyntaxMerge m = {
        .subsyn = subsyn,
        .return_state = NULL,
        .delim = NULL,
        .delim_len = 0,
    };

    if (streq(ret, "END")) {
        if (not_subsyntax(sp)) {
            return false;
        }
    } else if (subsyn) {
        m.return_state = reference_state(sp, ret);
    }

    if (subsyn) {
        *dest = merge_syntax(sp->current_syntax, &m, sp->colors);
        return true;
    }

    return false;
}

static bool destination_state(SyntaxParser *sp, const char *name, State **dest)
{
    const char *sep = strchr(name, ':');
    if (sep) {
        // subsyntax:returnstate
        char *sub = xstrcut(name, sep - name);
        bool success = subsyntax_call(sp, sub, sep + 1, dest);
        free(sub);
        return success;
    }

    if (streq(name, "END")) {
        if (not_subsyntax(sp)) {
            return false;
        }
        *dest = NULL;
        return true;
    }

    *dest = reference_state(sp, name);
    return true;
}

static Condition *add_condition (
    SyntaxParser *sp,
    ConditionType type,
    const char *dest,
    const char *emit
) {
    BUG_ON(!dest && cond_type_has_destination(type));
    if (no_state(sp)) {
        return NULL;
    }

    State *d = NULL;
    if (dest && !destination_state(sp, dest, &d)) {
        return NULL;
    }

    Condition *c = xnew0(Condition, 1);
    c->a.destination = d;
    c->a.emit_name = emit ? xstrdup(emit) : NULL;
    c->type = type;
    ptr_array_append(&sp->current_state->conds, c);
    return c;
}

static bool cmd_bufis(SyntaxParser *sp, const CommandArgs *a)
{
    const char *str = a->args[0];
    const size_t len = strlen(str);
    Condition *c;
    if (unlikely(len > ARRAYLEN(c->u.str.buf))) {
        error_msg (
            "Maximum length of string is %zu bytes",
            ARRAYLEN(c->u.str.buf)
        );
        return false;
    }

    ConditionType type = a->flags[0] == 'i' ? COND_BUFIS_ICASE : COND_BUFIS;
    c = add_condition(sp, type, a->args[1], a->args[2]);
    if (!c) {
        return false;
    }

    memcpy(c->u.str.buf, str, len);
    c->u.str.len = len;
    return true;
}

static bool cmd_char(SyntaxParser *sp, const CommandArgs *a)
{
    const char *chars = a->args[0];
    if (unlikely(chars[0] == '\0')) {
        error_msg("char argument can't be empty");
        return false;
    }

    bool add_to_buffer = cmdargs_has_flag(a, 'b');
    bool invert = cmdargs_has_flag(a, 'n');
    ConditionType type;
    if (add_to_buffer) {
        type = COND_CHAR_BUFFER;
    } else if (!invert && chars[1] == '\0') {
        type = COND_CHAR1;
    } else {
        type = COND_CHAR;
    }

    Condition *c = add_condition(sp, type, a->args[1], a->args[2]);
    if (!c) {
        return false;
    }

    if (type == COND_CHAR1) {
        c->u.ch = (unsigned char)chars[0];
    } else {
        bitset_add_char_range(c->u.bitset, chars);
        if (invert) {
            BITSET_INVERT(c->u.bitset);
        }
    }

    return true;
}

static bool cmd_default(SyntaxParser *sp, const CommandArgs *a)
{
    close_state(sp);
    if (no_syntax(sp)) {
        return false;
    }

    const char *value = str_intern(a->args[0]);
    HashMap *map = &sp->current_syntax->default_colors;
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *name = a->args[i];
        void *oldval = hashmap_insert_or_replace(map, xstrdup(name), (char*)value);
        if (unlikely(oldval)) {
            LOG_WARNING (
                "duplicate 'default' argument in %s:%d: '%s'",
                current_config.file,
                current_config.line,
                name
            );
        }
    }

    return true;
}

static bool cmd_eat(SyntaxParser *sp, const CommandArgs *a)
{
    if (no_state(sp)) {
        return false;
    }

    const char *dest = a->args[0];
    if (!destination_state(sp, dest, &sp->current_state->default_action.destination)) {
        return false;
    }

    const char *emit = a->args[1];
    sp->current_state->default_action.emit_name = emit ? xstrdup(emit) : NULL;
    sp->current_state->type = STATE_EAT;
    sp->current_state = NULL;
    return true;
}

static bool cmd_heredocbegin(SyntaxParser *sp, const CommandArgs *a)
{
    if (no_state(sp)) {
        return false;
    }

    Syntax *subsyn = must_find_subsyntax(sp, a->args[0]);
    if (!subsyn) {
        return false;
    }

    // default_action.destination is used as the return state
    const char *ret = a->args[1];
    if (!destination_state(sp, ret, &sp->current_state->default_action.destination)) {
        return false;
    }

    sp->current_state->default_action.emit_name = NULL;
    sp->current_state->type = STATE_HEREDOCBEGIN;
    sp->current_state->heredoc.subsyntax = subsyn;
    sp->current_state = NULL;

    // Normally merge() marks subsyntax used but in case of heredocs merge()
    // is not called when syntax file is loaded
    subsyn->used = true;
    return true;
}

static bool cmd_heredocend(SyntaxParser *sp, const CommandArgs *a)
{
    Condition *c = add_condition(sp, COND_HEREDOCEND, a->args[0], a->args[1]);
    if (unlikely(!c)) {
        return false;
    }
    BUG_ON(!sp->current_syntax);
    sp->current_syntax->heredoc = true;
    return true;
}

// Forward declaration, used in cmd_include() and cmd_require()
static int read_syntax(SyntaxParser *sp, const char *filename, ConfigFlags flags);

static bool cmd_include(SyntaxParser *sp, const CommandArgs *a)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (a->flags[0] == 'b') {
        flags |= CFG_BUILTIN;
    }
    int r = read_syntax(sp, a->args[0], flags);
    return r == 0;
}

static bool cmd_list(SyntaxParser *sp, const CommandArgs *a)
{
    close_state(sp);
    if (no_syntax(sp)) {
        return false;
    }

    char **args = a->args;
    const char *name = args[0];
    StringList *list = find_string_list(sp->current_syntax, name);
    if (!list) {
        list = xnew0(StringList, 1);
        hashmap_insert(&sp->current_syntax->string_lists, xstrdup(name), list);
    } else if (unlikely(list->defined)) {
        error_msg("List '%s' already exists", name);
        return false;
    }
    list->defined = true;

    bool icase = a->flags[0] == 'i';
    HashSet *set = &list->strings;
    hashset_init(set, a->nr_args - 1, icase);
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *str = args[i];
        hashset_add(set, str, strlen(str));
    }
    return true;
}

static bool cmd_inlist(SyntaxParser *sp, const CommandArgs *a)
{
    char **args = a->args;
    const char *name = args[0];
    const char *emit = args[2] ? args[2] : name;
    Condition *c = add_condition(sp, COND_INLIST, args[1], emit);
    if (!c) {
        return false;
    }

    StringList *list = find_string_list(sp->current_syntax, name);
    if (unlikely(!list)) {
        // Add undefined list
        list = xnew0(StringList, 1);
        hashmap_insert(&sp->current_syntax->string_lists, xstrdup(name), list);
    }

    list->used = true;
    c->u.str_list = list;
    return true;
}

static bool cmd_noeat(SyntaxParser *sp, const CommandArgs *a)
{
    State *dest;
    if (unlikely(no_state(sp) || !destination_state(sp, a->args[0], &dest))) {
        return false;
    }

    if (unlikely(dest == sp->current_state)) {
        error_msg("using noeat to jump to same state causes infinite loop");
        return false;
    }

    sp->current_state->default_action.destination = dest;
    sp->current_state->default_action.emit_name = NULL;
    sp->current_state->type = a->flags[0] == 'b' ? STATE_NOEAT_BUFFER : STATE_NOEAT;
    sp->current_state = NULL;
    return true;
}

static bool cmd_recolor(SyntaxParser *sp, const CommandArgs *a)
{
    // If length is not specified then buffered bytes will be recolored
    ConditionType type = COND_RECOLOR_BUFFER;
    size_t len = 0;

    const char *len_str = a->args[1];
    if (len_str) {
        type = COND_RECOLOR;
        if (unlikely(!str_to_size(len_str, &len))) {
            error_msg("invalid number: '%s'", len_str);
            return false;
        }
        if (unlikely(len < 1 || len > 2500)) {
            error_msg("number of bytes must be between 1-2500 (got %zu)", len);
            return false;
        }
    }

    Condition *c = add_condition(sp, type, NULL, a->args[0]);
    if (!c) {
        return false;
    }

    if (type == COND_RECOLOR) {
        c->u.recolor_len = len;
    }

    return true;
}

static bool cmd_require(SyntaxParser *sp, const CommandArgs *a)
{
    static HashSet loaded_files;
    static HashSet loaded_builtins;
    if (!loaded_files.table_size) {
        hashset_init(&loaded_files, 8, false);
        hashset_init(&loaded_builtins, 8, false);
    }

    char buf[4096];
    char *path;
    size_t path_len;
    HashSet *set;
    ConfigFlags flags = CFG_MUST_EXIST;

    if (a->flags[0] == 'f') {
        set = &loaded_files;
        path = a->args[0];
        path_len = strlen(path);
    } else {
        set = &loaded_builtins;
        path_len = xsnprintf(buf, sizeof(buf), "syntax/inc/%s", a->args[0]);
        path = buf;
        flags |= CFG_BUILTIN;
    }

    if (hashset_get(set, path, path_len)) {
        return true;
    }

    if (read_syntax(sp, path, flags) != 0) {
        return false;
    }

    hashset_add(set, path, path_len);
    return true;
}

static bool cmd_state(SyntaxParser *sp, const CommandArgs *a)
{
    close_state(sp);
    if (no_syntax(sp)) {
        return false;
    }

    const char *name = a->args[0];
    if (unlikely(streq(name, "END") || streq(name, "this"))) {
        error_msg("'%s' is reserved state name", name);
        return false;
    }

    State *state = find_or_add_state(sp, name);
    if (unlikely(state->defined)) {
        error_msg("State '%s' already exists", name);
        return false;
    }

    state->defined = true;
    state->emit_name = xstrdup(a->args[1] ? a->args[1] : a->args[0]);
    sp->current_state = state;
    return true;
}

static bool cmd_str(SyntaxParser *sp, const CommandArgs *a)
{
    const char *str = a->args[0];
    size_t len = strlen(str);
    if (unlikely(len < 2)) {
        error_msg("string should be at least 2 bytes; use 'char' for single bytes");
        return false;
    }

    Condition *c;
    size_t maxlen = ARRAYLEN(c->u.str.buf);
    if (unlikely(len > maxlen)) {
        error_msg("maximum length of string is %zu bytes", maxlen);
        return false;
    }

    ConditionType type;
    if (cmdargs_has_flag(a, 'i')) {
        type = COND_STR_ICASE;
    } else {
        type = (len == 2) ? COND_STR2 : COND_STR;
    }

    c = add_condition(sp, type, a->args[1], a->args[2]);
    if (!c) {
        return false;
    }

    memcpy(c->u.str.buf, str, len);
    c->u.str.len = len;
    return true;
}

static void finish_syntax(SyntaxParser *sp)
{
    BUG_ON(!sp->current_syntax);
    close_state(sp);
    finalize_syntax(sp->syntaxes, sp->current_syntax, sp->saved_nr_errors);
    sp->current_syntax = NULL;
}

static bool cmd_syntax(SyntaxParser *sp, const CommandArgs *a)
{
    if (sp->current_syntax) {
        finish_syntax(sp);
    }

    sp->current_syntax = xnew0(Syntax, 1);
    sp->current_syntax->name = xstrdup(a->args[0]);
    sp->current_state = NULL;

    sp->saved_nr_errors = get_nr_errors();
    return true;
}

IGNORE_WARNING("-Wincompatible-pointer-types")

static const Command cmds[] = {
    {"bufis", "i", true, 2, 3, cmd_bufis},
    {"char", "bn", true, 2, 3, cmd_char},
    {"default", "", true, 2, -1, cmd_default},
    {"eat", "", true, 1, 2, cmd_eat},
    {"heredocbegin", "", true, 2, 2, cmd_heredocbegin},
    {"heredocend", "", true, 1, 2, cmd_heredocend},
    {"include", "b", true, 1, 1, cmd_include},
    {"inlist", "", true, 2, 3, cmd_inlist},
    {"list", "i", true, 2, -1, cmd_list},
    {"noeat", "b", true, 1, 1, cmd_noeat},
    {"recolor", "", true, 1, 2, cmd_recolor},
    {"require", "f", true, 1, 1, cmd_require},
    {"state", "", true, 1, 2, cmd_state},
    {"str", "i", true, 2, 3, cmd_str},
    {"syntax", "", true, 1, 1, cmd_syntax},
};

UNIGNORE_WARNINGS

UNITTEST {
    CHECK_BSEARCH_ARRAY(cmds, name, strcmp);
}

static const Command *find_syntax_command(const char *name)
{
    return BSEARCH(name, cmds, command_cmp);
}

static bool expand_syntax_var(const char *name, char **value, const void *userdata)
{
    if (streq(name, "DTE_HOME")) {
        const SyntaxParser *sp = userdata;
        *value = xstrdup(sp->user_config_dir);
        return true;
    }
    return false;
}

static const CommandSet syntax_commands = {
    .lookup = find_syntax_command,
    .expand_variable = expand_syntax_var,
};

static CommandRunner cmdrunner_for_syntaxes(SyntaxParser *sp)
{
    CommandRunner runner = {
        .cmds = &syntax_commands,
        .home_dir = sp->home_dir,
        .userdata = sp,
    };
    return runner;
}

static int read_syntax(SyntaxParser *sp, const char *filename, ConfigFlags flags)
{
    CommandRunner runner = cmdrunner_for_syntaxes(sp);
    return read_config(&runner, filename, flags);
}

Syntax *load_syntax_file(EditorState *e, const char *filename, ConfigFlags flags, int *err)
{
    SyntaxParser sp = {
        .home_dir = &e->home_dir,
        .user_config_dir = e->user_config_dir,
        .colors = &e->colors,
        .syntaxes = &e->syntaxes,
        .current_syntax = NULL,
        .current_state = NULL,
        .saved_nr_errors = 0,
    };

    const ConfigState saved = current_config;
    CommandRunner runner = cmdrunner_for_syntaxes(&sp);
    *err = do_read_config(&runner, filename, flags);
    if (*err) {
        current_config = saved;
        return NULL;
    }

    if (sp.current_syntax) {
        finish_syntax(&sp);
        find_unused_subsyntaxes(sp.syntaxes);
    }

    current_config = saved;

    Syntax *syn = find_syntax(sp.syntaxes, path_basename(filename));
    if (!syn) {
        *err = EINVAL;
        return NULL;
    }

    if (e->status != EDITOR_INITIALIZING) {
        update_syntax_colors(syn, sp.colors);
    }

    return syn;
}

Syntax *load_syntax_by_filetype(EditorState *e, const char *filetype)
{
    if (!is_valid_filetype_name(filetype)) {
        return NULL;
    }

    const char *cfgdir = e->user_config_dir;
    char filename[4096];
    int err;

    xsnprintf(filename, sizeof filename, "%s/syntax/%s", cfgdir, filetype);
    Syntax *syn = load_syntax_file(e, filename, CFG_NOFLAGS, &err);
    if (syn || err != ENOENT) {
        return syn;
    }

    xsnprintf(filename, sizeof filename, "syntax/%s", filetype);
    return load_syntax_file(e, filename, CFG_BUILTIN, &err);
}
