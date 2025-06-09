#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "state.h"
#include "command/args.h"
#include "command/error.h"
#include "command/run.h"
#include "config.h"
#include "editor.h"
#include "filetype.h"
#include "syntax/merge.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/hashset.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"
#include "util/xstring.h"

static bool in_syntax(EditorState *e)
{
    return likely(e->syn.current_syntax) || error_msg(&e->err, "No syntax started");
}

static bool in_state(EditorState *e)
{
    if (unlikely(!in_syntax(e))) {
        return false;
    }
    return likely(e->syn.current_state) || error_msg(&e->err, "No state started");
}

static void close_state(EditorState *e)
{
    const State *state = e->syn.current_state;
    if (!state) {
        return;
    }

    if (unlikely(state->type == STATE_INVALID)) {
        // This error applies to the state itself rather than the last
        // command, so it doesn't make sense to include the command name
        // in the error message
        error_msg_for_cmd(&e->err, NULL, "No default action in state '%s'", state->name);
    }

    e->syn.current_state = NULL;
}

static State *find_or_add_state(EditorState *e, const char *name)
{
    State *state = find_state(e->syn.current_syntax, name);
    if (state) {
        return state;
    }

    state = xcalloc1(sizeof(*state));
    state->name = xstrdup(name);
    state->defined = false;
    state->type = STATE_INVALID;

    if (e->syn.current_syntax->states.count == 0) {
        e->syn.current_syntax->start_state = state;
    }

    return hashmap_insert(&e->syn.current_syntax->states, state->name, state);
}

static State *reference_state(EditorState *e, const char *name)
{
    if (streq(name, "this")) {
        return e->syn.current_state;
    }

    State *state = find_or_add_state(e, name);
    if (unlikely((e->syn.flags & SYN_LINT) && state == e->syn.current_state)) {
        error_msg (
            &e->err,
            "destination '%s' can be optimized to 'this' in '%s' syntax",
            name,
            e->syn.current_syntax->name
        );
    }

    return state;
}

static bool in_subsyntax(EditorState *e)
{
    bool ss = likely(is_subsyntax(e->syn.current_syntax));
    return ss || error_msg(&e->err, "Destination state 'END' only allowed in a subsyntax");
}

static Syntax *must_find_subsyntax(EditorState *e, const char *name)
{
    Syntax *syntax = find_any_syntax(&e->syntaxes, name);
    if (unlikely(!syntax)) {
        error_msg(&e->err, "No such syntax '%s'", name);
        return NULL;
    }
    if (unlikely(!is_subsyntax(syntax))) {
        error_msg(&e->err, "Syntax '%s' is not a subsyntax", name);
        return NULL;
    }
    return syntax;
}

static bool subsyntax_call(EditorState *e, const char *name, const char *ret, State **dest)
{
    Syntax *subsyn = must_find_subsyntax(e, name);

    SyntaxMerge m = {
        .subsyn = subsyn,
        .return_state = NULL,
        .delim = NULL,
        .delim_len = 0,
    };

    if (streq(ret, "END")) {
        if (!in_subsyntax(e)) {
            return false;
        }
    } else if (subsyn) {
        m.return_state = reference_state(e, ret);
    }

    if (subsyn) {
        *dest = merge_syntax(e->syn.current_syntax, &m, &e->styles);
        return true;
    }

    return false;
}

static bool destination_state(EditorState *e, const char *name, State **dest)
{
    const char *sep = strchr(name, ':');
    if (sep) {
        // subsyntax:returnstate
        char *sub = xstrcut(name, sep - name);
        bool success = subsyntax_call(e, sub, sep + 1, dest);
        free(sub);
        return success;
    }

    if (streq(name, "END")) {
        if (!in_subsyntax(e)) {
            return false;
        }
        *dest = NULL;
        return true;
    }

    *dest = reference_state(e, name);
    return true;
}

static void lint_emit_name(EditorState *e, const char *ename, const State *dest)
{
    if (
        (e->syn.flags & SYN_LINT)
        && ename
        && dest
        && dest->emit_name
        && interned_strings_equal(ename, dest->emit_name)
    ) {
        error_msg (
            &e->err,
            "emit-name '%s' not needed (destination state uses same emit-name)",
            ename
        );
    }
}

static Condition *add_condition (
    EditorState *e,
    ConditionType type,
    const char *dest,
    const char *emit
) {
    BUG_ON(!dest && cond_type_has_destination(type));
    if (!in_state(e)) {
        return NULL;
    }

    State *d = NULL;
    if (dest && !destination_state(e, dest, &d)) {
        return NULL;
    }

    emit = emit ? str_intern(emit) : NULL;

    if (
        type != COND_HEREDOCEND
        && type != COND_INLIST
        && type != COND_INLIST_BUFFER
    ) {
        lint_emit_name(e, emit, d);
    }

    Condition *c = xcalloc1(sizeof(*c));
    c->a.destination = d;
    c->a.emit_name = emit;
    c->type = type;
    ptr_array_append(&e->syn.current_state->conds, c);
    return c;
}

static bool cmd_bufis(EditorState *e, const CommandArgs *a)
{
    const char *str = a->args[0];
    const size_t len = strlen(str);
    Condition *c;
    if (unlikely(len > ARRAYLEN(c->u.str.buf))) {
        return error_msg (
            &e->err,
            "Maximum length of string is %zu bytes",
            ARRAYLEN(c->u.str.buf)
        );
    }

    ConditionType type = a->flags[0] == 'i' ? COND_BUFIS_ICASE : COND_BUFIS;
    c = add_condition(e, type, a->args[1], a->args[2]);
    if (!c) {
        return false;
    }

    memcpy(c->u.str.buf, str, len);
    c->u.str.len = len;
    return true;
}

static bool cmd_char(EditorState *e, const CommandArgs *a)
{
    const char *chars = a->args[0];
    if (unlikely(chars[0] == '\0')) {
        return error_msg(&e->err, "char argument can't be empty");
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

    Condition *c = add_condition(e, type, a->args[1], a->args[2]);
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

static bool cmd_default(EditorState *e, const CommandArgs *a)
{
    close_state(e);
    if (!in_syntax(e)) {
        return false;
    }

    const char *value = str_intern(a->args[0]);
    HashMap *map = &e->syn.current_syntax->default_styles;
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *name = a->args[i];
        const void *oldval = hashmap_insert_or_replace(map, xstrdup(name), (char*)value);
        if (unlikely(oldval)) {
            error_msg(&e->err, "'%s' argument specified multiple times", name);
        }
    }

    return true;
}

static bool cmd_eat(EditorState *e, const CommandArgs *a)
{
    if (!in_state(e)) {
        return false;
    }

    const char *dest = a->args[0];
    if (!destination_state(e, dest, &e->syn.current_state->default_action.destination)) {
        return false;
    }

    const char *emit = a->args[1] ? str_intern(a->args[1]) : NULL;
    State *curstate = e->syn.current_state;
    lint_emit_name(e, emit, curstate->default_action.destination);
    curstate->default_action.emit_name = emit;
    curstate->type = STATE_EAT;
    e->syn.current_state = NULL;
    return true;
}

static bool cmd_heredocbegin(EditorState *e, const CommandArgs *a)
{
    if (!in_state(e)) {
        return false;
    }

    Syntax *subsyn = must_find_subsyntax(e, a->args[0]);
    if (!subsyn) {
        return false;
    }

    // default_action.destination is used as the return state
    const char *ret = a->args[1];
    if (!destination_state(e, ret, &e->syn.current_state->default_action.destination)) {
        return false;
    }

    e->syn.current_state->default_action.emit_name = NULL;
    e->syn.current_state->type = STATE_HEREDOCBEGIN;
    e->syn.current_state->heredoc.subsyntax = subsyn;
    e->syn.current_state = NULL;

    // Normally merge() marks subsyntax used but in case of heredocs merge()
    // is not called when syntax file is loaded
    subsyn->used = true;
    return true;
}

static bool cmd_heredocend(EditorState *e, const CommandArgs *a)
{
    Condition *c = add_condition(e, COND_HEREDOCEND, a->args[0], a->args[1]);
    if (unlikely(!c)) {
        return false;
    }
    BUG_ON(!e->syn.current_syntax);
    e->syn.current_syntax->heredoc = true;
    return true;
}

// Forward declaration, used in cmd_include() and cmd_require()
static int read_syntax(EditorState *e, const char *filename, SyntaxLoadFlags flags);

static bool cmd_include(EditorState *e, const CommandArgs *a)
{
    SyntaxLoadFlags flags = SYN_MUST_EXIST;
    if (a->flags[0] == 'b') {
        flags |= SYN_BUILTIN;
    }
    int r = read_syntax(e, a->args[0], flags);
    return r == 0;
}

static bool cmd_list(EditorState *e, const CommandArgs *a)
{
    close_state(e);
    if (!in_syntax(e)) {
        return false;
    }

    char **args = a->args;
    const char *name = args[0];
    StringList *list = find_string_list(e->syn.current_syntax, name);
    if (!list) {
        list = xcalloc1(sizeof(*list));
        hashmap_insert(&e->syn.current_syntax->string_lists, xstrdup(name), list);
    } else if (unlikely(list->defined)) {
        return error_msg(&e->err, "List '%s' already exists", name);
    }
    list->defined = true;

    bool icase = a->flags[0] == 'i';
    HashSet *set = &list->strings;
    hashset_init(set, a->nr_args - 1, icase);
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *str = args[i];
        hashset_insert(set, str, strlen(str));
    }
    return true;
}

static bool cmd_inlist(EditorState *e, const CommandArgs *a)
{
    char **args = a->args;
    const char *name = args[0];
    ConditionType type = cmdargs_has_flag(a, 'b') ? COND_INLIST_BUFFER : COND_INLIST;
    Condition *c = add_condition(e, type, args[1], args[2] ? args[2] : name);

    if (!c) {
        return false;
    }

    StringList *list = find_string_list(e->syn.current_syntax, name);
    if (unlikely(!list)) {
        // Add undefined list
        list = xcalloc1(sizeof(*list));
        hashmap_insert(&e->syn.current_syntax->string_lists, xstrdup(name), list);
    }

    list->used = true;
    c->u.str_list = list;
    return true;
}

static bool cmd_noeat(EditorState *e, const CommandArgs *a)
{
    State *dest;
    if (unlikely(!in_state(e) || !destination_state(e, a->args[0], &dest))) {
        return false;
    }

    if (unlikely(dest == e->syn.current_state)) {
        return error_msg(&e->err, "using noeat to jump to same state causes infinite loop");
    }

    e->syn.current_state->default_action.destination = dest;
    e->syn.current_state->default_action.emit_name = NULL;
    e->syn.current_state->type = a->flags[0] == 'b' ? STATE_NOEAT_BUFFER : STATE_NOEAT;
    e->syn.current_state = NULL;
    return true;
}

static bool cmd_recolor(EditorState *e, const CommandArgs *a)
{
    // If length is not specified then buffered bytes will be recolored
    ConditionType type = COND_RECOLOR_BUFFER;
    size_t len = 0;

    const char *len_str = a->args[1];
    if (len_str) {
        type = COND_RECOLOR;
        if (unlikely(!str_to_size(len_str, &len))) {
            return error_msg(&e->err, "invalid number: '%s'", len_str);
        }
        if (unlikely(len < 1 || len > 2500)) {
            return error_msg(&e->err, "number of bytes must be between 1-2500 (got %zu)", len);
        }
    }

    Condition *c = add_condition(e, type, NULL, a->args[0]);
    if (!c) {
        return false;
    }

    if (type == COND_RECOLOR) {
        c->u.recolor_len = len;
    }

    return true;
}

static bool cmd_require(EditorState *e, const CommandArgs *a)
{
    char buf[8192];
    char *path;
    size_t path_len;
    HashSet *set;
    SyntaxLoadFlags flags = SYN_MUST_EXIST;

    if (a->flags[0] == 'f') {
        set = &e->required_syntax_files;
        path = a->args[0];
        path_len = strlen(path);
    } else {
        set = &e->required_syntax_builtins;
        path_len = xsnprintf(buf, sizeof(buf), "syntax/inc/%s", a->args[0]);
        path = buf;
        flags |= SYN_BUILTIN;
    }

    if (hashset_get(set, path, path_len)) {
        return true;
    }

    const SyntaxLoadFlags save = e->syn.flags;
    e->syn.flags &= ~SYN_WARN_ON_UNUSED_SUBSYN;
    int r = read_syntax(e, path, flags);
    e->syn.flags = save;
    if (r != 0) {
        return false;
    }

    hashset_insert(set, path, path_len);
    return true;
}

static bool cmd_state(EditorState *e, const CommandArgs *a)
{
    close_state(e);
    if (!in_syntax(e)) {
        return false;
    }

    const char *name = a->args[0];
    if (unlikely(streq(name, "END") || streq(name, "this"))) {
        return error_msg(&e->err, "'%s' is reserved state name", name);
    }

    State *state = find_or_add_state(e, name);
    if (unlikely(state->defined)) {
        return error_msg(&e->err, "State '%s' already exists", name);
    }

    state->defined = true;
    state->emit_name = str_intern(a->args[1] ? a->args[1] : name);
    e->syn.current_state = state;
    return true;
}

static bool cmd_str(EditorState *e, const CommandArgs *a)
{
    const char *str = a->args[0];
    size_t len = strlen(str);
    if (unlikely(len < 2)) {
        return error_msg(&e->err, "string should be at least 2 bytes; use 'char' for single bytes");
    }

    Condition *c;
    size_t maxlen = ARRAYLEN(c->u.str.buf);
    if (unlikely(len > maxlen)) {
        return error_msg(&e->err, "maximum length of string is %zu bytes", maxlen);
    }

    ConditionType type;
    if (cmdargs_has_flag(a, 'i')) {
        type = COND_STR_ICASE;
    } else {
        type = (len == 2) ? COND_STR2 : COND_STR;
    }

    c = add_condition(e, type, a->args[1], a->args[2]);
    if (!c) {
        return false;
    }

    memcpy(c->u.str.buf, str, len);
    c->u.str.len = len;
    return true;
}

static void finish_syntax(EditorState *e)
{
    BUG_ON(!e->syn.current_syntax);
    close_state(e);
    finalize_syntax(&e->syntaxes, e->syn.current_syntax, &e->err, e->syn.saved_nr_errors);
    e->syn.current_syntax = NULL;
}

static bool cmd_syntax(EditorState *e, const CommandArgs *a)
{
    if (e->syn.current_syntax) {
        finish_syntax(e);
    }

    Syntax *syntax = xcalloc1(sizeof(*syntax));
    syntax->name = xstrdup(a->args[0]);
    if (is_subsyntax(syntax) && !(e->syn.flags & SYN_WARN_ON_UNUSED_SUBSYN)) {
        syntax->warned_unused_subsyntax = true;
    }

    e->syn.current_syntax = syntax;
    e->syn.current_state = NULL;
    e->syn.saved_nr_errors = e->err.nr_errors;
    return true;
}

#define CMD(name, flags, min, max, func) \
    {name, flags, CMDOPT_ALLOW_IN_RC, min, max, func}

static const Command cmds[] = {
    CMD("bufis", "i", 2, 3, cmd_bufis),
    CMD("char", "bn", 2, 3, cmd_char),
    CMD("default", "", 2, -1, cmd_default),
    CMD("eat", "", 1, 2, cmd_eat),
    CMD("heredocbegin", "", 2, 2, cmd_heredocbegin),
    CMD("heredocend", "", 1, 2, cmd_heredocend),
    CMD("include", "b", 1, 1, cmd_include),
    CMD("inlist", "b", 2, 3, cmd_inlist),
    CMD("list", "i", 2, -1, cmd_list),
    CMD("noeat", "b", 1, 1, cmd_noeat),
    CMD("recolor", "", 1, 2, cmd_recolor),
    CMD("require", "f", 1, 1, cmd_require),
    CMD("state", "", 1, 2, cmd_state),
    CMD("str", "i", 2, 3, cmd_str),
    CMD("syntax", "", 1, 1, cmd_syntax),
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(cmds, name);
}

static const Command *find_syntax_command(const char *name)
{
    return BSEARCH(name, cmds, command_cmp);
}

static char *expand_syntax_var(const EditorState *e, const char *name)
{
    if (streq(name, "DTE_HOME")) {
        return xstrdup(e->user_config_dir);
    }
    return NULL;
}

static const CommandSet syntax_commands = {
    .lookup = find_syntax_command,
};

static CommandRunner cmdrunner_for_syntaxes(EditorState *e)
{
    CommandRunner runner = cmdrunner(e, &syntax_commands);
    runner.expand_variable = expand_syntax_var;
    return runner;
}

static ConfigFlags syn_flags_to_cfg_flags(SyntaxLoadFlags flags)
{
    static_assert(SYN_MUST_EXIST == (SyntaxLoadFlags)CFG_MUST_EXIST);
    static_assert(SYN_BUILTIN == (SyntaxLoadFlags)CFG_BUILTIN);
    SyntaxLoadFlags mask = SYN_MUST_EXIST | SYN_BUILTIN;
    return (ConfigFlags)(flags & mask);
}

static int read_syntax(EditorState *e, const char *filename, SyntaxLoadFlags flags)
{
    CommandRunner runner = cmdrunner_for_syntaxes(e);
    return read_config(&runner, filename, syn_flags_to_cfg_flags(flags));
}

Syntax *load_syntax_file(EditorState *e, const char *filename, SyntaxLoadFlags flags, int *err)
{
    e->syn = (SyntaxLoader) {
        .current_syntax = NULL,
        .current_state = NULL,
        .flags = flags | SYN_WARN_ON_UNUSED_SUBSYN,
        .saved_nr_errors = 0,
    };

    const char *saved_file = e->err.config_filename;
    const unsigned int saved_line = e->err.config_line;
    CommandRunner runner = cmdrunner_for_syntaxes(e);
    *err = do_read_config(&runner, filename, syn_flags_to_cfg_flags(flags));

    if (!*err && e->syn.current_syntax) {
        finish_syntax(e);
        find_unused_subsyntaxes(&e->syntaxes, &e->err);
    }

    e->err.config_filename = saved_file;
    e->err.config_line = saved_line;

    if (*err) {
        return NULL;
    }

    Syntax *syn = find_syntax(&e->syntaxes, path_basename(filename));
    if (!syn) {
        *err = EINVAL;
        return NULL;
    }

    if (e->status != EDITOR_INITIALIZING) {
        update_syntax_styles(syn, &e->styles);
    }

    return syn;
}

Syntax *load_syntax_by_filetype(EditorState *e, const char *filetype)
{
    if (!is_valid_filetype_name(filetype)) {
        return NULL;
    }

    const char *cfgdir = e->user_config_dir;
    char filename[8192];
    xsnprintf(filename, sizeof filename, "%s/syntax/%s", cfgdir, filetype);

    int err;
    Syntax *syn = load_syntax_file(e, filename, 0, &err);
    if (syn || err != ENOENT) {
        return syn;
    }

    // Skip past "%s/" (cfgdir) part of formatted string from above and
    // try to load a built-in syntax named `syntax/<filetype>`
    const char *builtin_name = filename + strlen(cfgdir) + STRLEN("/");
    return load_syntax_file(e, builtin_name, SYN_BUILTIN, &err);
}
