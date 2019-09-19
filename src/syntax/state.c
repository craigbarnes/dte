#include <errno.h>
#include "state.h"
#include "syntax.h"
#include "../command.h"
#include "../config.h"
#include "../editor.h"
#include "../error.h"
#include "../parse-args.h"
#include "../terminal/color.h"
#include "../util/path.h"
#include "../util/str-util.h"
#include "../util/strtonum.h"
#include "../util/xmalloc.h"
#include "../util/xsnprintf.h"

static Syntax *current_syntax;
static State *current_state;
static unsigned int saved_nr_errors; // Used to check if nr_errors changed

static bool no_syntax(void)
{
    if (current_syntax) {
        return false;
    }
    error_msg("No syntax started");
    return true;
}

static bool no_state(void)
{
    if (no_syntax()) {
        return true;
    }
    if (current_state) {
        return false;
    }
    error_msg("No state started");
    return true;
}

static void close_state(void)
{
    if (current_state && current_state->type == STATE_INVALID) {
        // Command prefix in error message makes no sense
        const Command *save = current_command;
        current_command = NULL;
        error_msg("No default action in state %s", current_state->name);
        current_command = save;
    }
    current_state = NULL;
}

static State *find_or_add_state(const char *name)
{
    State *st = find_state(current_syntax, name);
    if (st) {
        return st;
    }

    st = xnew0(State, 1);
    st->name = xstrdup(name);
    st->defined = false;
    st->type = STATE_INVALID;
    ptr_array_add(&current_syntax->states, st);
    return st;
}

static State *reference_state(const char *name)
{
    if (streq(name, "this")) {
        return current_state;
    }
    return find_or_add_state(name);
}

static bool not_subsyntax(void)
{
    if (is_subsyntax(current_syntax)) {
        return false;
    }
    error_msg("Destination state END allowed only in a subsyntax.");
    return true;
}

static bool subsyntax_call(const char *name, const char *ret, State **dest)
{
    SyntaxMerge m = {
        .subsyn = find_any_syntax(name),
        .return_state = NULL,
        .delim = NULL,
        .delim_len = 0,
    };
    bool ok = true;

    if (!m.subsyn) {
        error_msg("No such syntax %s", name);
        ok = false;
    } else if (!is_subsyntax(m.subsyn)) {
        error_msg("Syntax %s is not subsyntax", name);
        ok = false;
    }
    if (streq(ret, "END")) {
        if (not_subsyntax()) {
            ok = false;
        }
    } else if (ok) {
        m.return_state = reference_state(ret);
    }
    if (ok) {
        *dest = merge_syntax(current_syntax, &m);
    }
    return ok;
}

static bool destination_state(const char *name, State **dest)
{
    const char *const sep = strchr(name, ':');
    if (sep) {
        // subsyntax:returnstate
        char *sub = xstrcut(name, sep - name);
        bool success = subsyntax_call(sub, sep + 1, dest);
        free(sub);
        return success;
    }

    if (streq(name, "END")) {
        if (not_subsyntax()) {
            return false;
        }
        *dest = NULL;
        return true;
    }

    *dest = reference_state(name);
    return true;
}

static Condition *add_condition (
    ConditionType type,
    const char *dest,
    const char *emit
) {
    if (no_state()) {
        return NULL;
    }

    State *d = NULL;
    if (dest && !destination_state(dest, &d)) {
        return NULL;
    }

    Condition *c = xnew0(Condition, 1);
    c->a.destination = d;
    c->a.emit_name = emit ? xstrdup(emit) : NULL;
    c->type = type;
    ptr_array_add(&current_state->conds, c);
    return c;
}

static void cmd_bufis(const CommandArgs *a)
{
    char **args = a->args;
    const bool icase = a->flags[0] == 'i';
    const char *str = args[0];
    const size_t len = strlen(str);
    Condition *c;

    if (len > ARRAY_COUNT(c->u.cond_bufis.str)) {
        error_msg (
            "Maximum length of string is %zu bytes",
            ARRAY_COUNT(c->u.cond_bufis.str)
        );
        return;
    }

    c = add_condition(COND_BUFIS, args[1], args[2]);
    if (c) {
        memcpy(c->u.cond_bufis.str, str, len);
        c->u.cond_bufis.len = len;
        c->u.cond_bufis.icase = icase;
    }
}

static void cmd_char(const CommandArgs *a)
{
    const char *pf = a->flags;
    bool n_flag = false;
    bool b_flag = false;
    while (*pf) {
        switch (*pf) {
        case 'b':
            b_flag = true;
            break;
        case 'n':
            n_flag = true;
            break;
        }
        pf++;
    }

    char **args = a->args;
    ConditionType type;
    if (b_flag) {
        type = COND_CHAR_BUFFER;
    } else if (!n_flag && args[0][0] != '\0' && args[0][1] == '\0') {
        type = COND_CHAR1;
    } else {
        type = COND_CHAR;
    }

    Condition *c = add_condition(type, args[1], args[2]);
    if (!c) {
        return;
    }

    if (type == COND_CHAR1) {
        c->u.cond_single_char.ch = (unsigned char)args[0][0];
    } else {
        bitset_add_pattern(c->u.cond_char.bitset, args[0]);
        if (n_flag) {
            bitset_invert(c->u.cond_char.bitset);
        }
    }
}

static void cmd_default(const CommandArgs *a)
{
    close_state();
    if (no_syntax()) {
        return;
    }
    ptr_array_add (
        &current_syntax->default_colors,
        copy_string_array(a->args, a->nr_args)
    );
}

static void cmd_eat(const CommandArgs *a)
{
    if (no_state()) {
        return;
    }

    if (!destination_state(a->args[0], &current_state->a.destination)) {
        return;
    }

    current_state->type = STATE_EAT;
    current_state->a.emit_name = a->args[1] ? xstrdup(a->args[1]) : NULL;
    current_state = NULL;
}

static void cmd_heredocbegin(const CommandArgs *a)
{
    if (no_state()) {
        return;
    }

    const char *sub = a->args[0];
    Syntax *subsyn = find_any_syntax(sub);
    if (!subsyn) {
        error_msg("No such syntax %s", sub);
        return;
    }
    if (!is_subsyntax(subsyn)) {
        error_msg("Syntax %s is not subsyntax", sub);
        return;
    }

    // a.destination is used as the return state
    if (!destination_state(a->args[1], &current_state->a.destination)) {
        return;
    }

    current_state->a.emit_name = NULL;
    current_state->type = STATE_HEREDOCBEGIN;
    current_state->heredoc.subsyntax = subsyn;
    current_state = NULL;

    // Normally merge() marks subsyntax used but in case of heredocs merge()
    // is not called when syntax file is loaded.
    subsyn->used = true;
}

static void cmd_heredocend(const CommandArgs *a)
{
    add_condition(COND_HEREDOCEND, a->args[0], a->args[1]);
    current_syntax->heredoc = true;
}

static void cmd_list(const CommandArgs *a)
{
    close_state();
    if (no_syntax()) {
        return;
    }

    char **args = a->args;
    const char *name = args[0];
    StringList *list = find_string_list(current_syntax, name);
    if (list == NULL) {
        list = xnew0(StringList, 1);
        list->name = xstrdup(name);
        ptr_array_add(&current_syntax->string_lists, list);
    } else if (list->defined) {
        error_msg("List %s already exists.", name);
        return;
    }
    list->defined = true;

    bool icase = a->flags[0] == 'i';
    size_t nstrings = a->nr_args - 1;
    hashset_init(&list->strings, nstrings, icase);
    hashset_add_many(&list->strings, args + 1, nstrings);
}

static void cmd_inlist(const CommandArgs *a)
{
    char **args = a->args;
    const char *name = args[0];
    const char *emit = args[2] ? args[2] : name;
    StringList *list = find_string_list(current_syntax, name);
    Condition *c = add_condition(COND_INLIST, args[1], emit);

    if (c == NULL) {
        return;
    }

    if (list == NULL) {
        // Add undefined list
        list = xnew0(StringList, 1);
        list->name = xstrdup(name);
        ptr_array_add(&current_syntax->string_lists, list);
    }
    list->used = true;
    c->u.cond_inlist.list = list;
}

static void cmd_noeat(const CommandArgs *a)
{
    if (no_state()) {
        return;
    }

    const char *arg = a->args[0];
    if (streq(arg, current_state->name)) {
        error_msg (
            "Using noeat to to jump to parent state causes"
            " infinite loop"
        );
        return;
    }

    State *dest;
    if (!destination_state(arg, &dest)) {
        return;
    }
    if (dest == current_state) {
        // The noeat command doesn't consume anything, so jumping to
        // the same state will always produce an infinite loop.
        current_state->type = STATE_INVALID;
        return;
    }

    current_state->a.destination = dest;
    current_state->a.emit_name = NULL;
    current_state->type = a->flags[0] == 'b' ? STATE_NOEAT_BUFFER : STATE_NOEAT;
    current_state = NULL;
}

static void cmd_recolor(const CommandArgs *a)
{
    // If length is not specified then buffered bytes will be recolored
    ConditionType type = COND_RECOLOR_BUFFER;
    size_t len = 0;

    const char *len_str = a->args[1];
    if (len_str) {
        type = COND_RECOLOR;
        if (!str_to_size(len_str, &len)) {
            error_msg("invalid number: %s", len_str);
            return;
        }
        if (len == 0) {
            error_msg("number of bytes must be larger than 0");
            return;
        }
        if (len > 2500) {
            error_msg("number of bytes cannot be larger than 2500");
            return;
        }
    }

    Condition *c = add_condition(type, NULL, a->args[0]);
    if (c && type == COND_RECOLOR) {
        c->u.cond_recolor.len = len;
    }
}

static void cmd_state(const CommandArgs *a)
{
    close_state();
    if (no_syntax()) {
        return;
    }

    const char *name = a->args[0];
    if (streq(name, "END") || streq(name, "this")) {
        error_msg("%s is reserved state name", name);
        return;
    }

    State *s = find_or_add_state(name);
    if (s->defined) {
        error_msg("State %s already exists.", name);
        return;
    }
    s->defined = true;
    s->emit_name = xstrdup(a->args[1] ? a->args[1] : a->args[0]);
    current_state = s;
}

static void cmd_str(const CommandArgs *a)
{
    bool icase = a->flags[0] == 'i';
    ConditionType type = icase ? COND_STR_ICASE : COND_STR;
    const char *str = a->args[0];
    Condition *c;
    size_t len = strlen(str);

    if (len > ARRAY_COUNT(c->u.cond_str.str)) {
        error_msg (
            "Maximum length of string is %zu bytes",
            ARRAY_COUNT(c->u.cond_str.str)
        );
        return;
    }

    // Strings of length 2 are very common
    if (!icase && len == 2) {
        type = COND_STR2;
    }
    c = add_condition(type, a->args[1], a->args[2]);
    if (c) {
        memcpy(c->u.cond_str.str, str, len);
        c->u.cond_str.len = len;
    }
}

static void finish_syntax(void)
{
    close_state();
    finalize_syntax(current_syntax, saved_nr_errors);
    current_syntax = NULL;
}

static void cmd_syntax(const CommandArgs *a)
{
    if (current_syntax) {
        finish_syntax();
    }

    current_syntax = xnew0(Syntax, 1);
    current_syntax->name = xstrdup(a->args[0]);
    current_state = NULL;

    saved_nr_errors = get_nr_errors();
}

static void cmd_include(const CommandArgs *a);

// Prevent Clang whining about .max_args = -1
#if HAS_WARNING("-Wbitfield-constant-conversion")
 IGNORE_WARNING("-Wbitfield-constant-conversion")
#endif

static const Command syntax_commands[] = {
    {"bufis", "i", 2, 3, cmd_bufis},
    {"char", "bn", 2, 3, cmd_char},
    {"default", "", 2, -1, cmd_default},
    {"eat", "", 1, 2, cmd_eat},
    {"heredocbegin", "", 2, 2, cmd_heredocbegin},
    {"heredocend", "", 1, 2, cmd_heredocend},
    {"include", "b", 1, 1, cmd_include},
    {"inlist", "", 2, 3, cmd_inlist},
    {"list", "i", 2, -1, cmd_list},
    {"noeat", "b", 1, 1, cmd_noeat},
    {"recolor", "", 1, 2, cmd_recolor},
    {"state", "", 1, 2, cmd_state},
    {"str", "i", 2, 3, cmd_str},
    {"syntax", "", 1, 1, cmd_syntax},
    {"", "", 0, 0, NULL}
};

UNIGNORE_WARNINGS

static void cmd_include(const CommandArgs *a)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (a->flags[0] == 'b') {
        flags |= CFG_BUILTIN;
    }
    read_config(syntax_commands, a->args[0], flags);
}

Syntax *load_syntax_file(const char *filename, ConfigFlags flags, int *err)
{
    const char *saved_config_file = config_file;
    int saved_config_line = config_line;

    *err = do_read_config(syntax_commands, filename, flags);
    if (*err) {
        config_file = saved_config_file;
        config_line = saved_config_line;
        return NULL;
    }
    if (current_syntax) {
        finish_syntax();
        find_unused_subsyntaxes();
    }
    config_file = saved_config_file;
    config_line = saved_config_line;

    Syntax *syn = find_syntax(path_basename(filename));
    if (syn && editor.status != EDITOR_INITIALIZING) {
        update_syntax_colors(syn);
    }
    if (syn == NULL) {
        *err = EINVAL;
    }
    return syn;
}

Syntax *load_syntax_by_filetype(const char *filetype)
{
    const char *cfgdir = editor.user_config_dir;
    char filename[4096];
    int err;

    xsnprintf(filename, sizeof filename, "%s/syntax/%s", cfgdir, filetype);
    Syntax *syn = load_syntax_file(filename, CFG_NOFLAGS, &err);
    if (syn || err != ENOENT) {
        return syn;
    }

    xsnprintf(filename, sizeof filename, "syntax/%s", filetype);
    return load_syntax_file(filename, CFG_BUILTIN, &err);
}
