#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "state.h"
#include "command/args.h"
#include "command/run.h"
#include "editor.h"
#include "error.h"
#include "util/bsearch.h"
#include "util/macros.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/strtonum.h"
#include "util/xmalloc.h"
#include "util/xsnprintf.h"

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

    hashmap_insert(&current_syntax->states, st->name, st);
    if (current_syntax->states.count == 1) {
        current_syntax->start_state = st;
    }

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
    const char *sep = strchr(name, ':');
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
    ptr_array_append(&current_state->conds, c);
    return c;
}

static void cmd_bufis(const CommandArgs *a)
{
    const char *str = a->args[0];
    const size_t len = strlen(str);
    Condition *c;
    if (len > ARRAY_COUNT(c->u.str.buf)) {
        error_msg (
            "Maximum length of string is %zu bytes",
            ARRAY_COUNT(c->u.str.buf)
        );
        return;
    }

    ConditionType type = a->flags[0] == 'i' ? COND_BUFIS_ICASE : COND_BUFIS;
    c = add_condition(type, a->args[1], a->args[2]);
    if (c) {
        memcpy(c->u.str.buf, str, len);
        c->u.str.len = len;
    }
}

static void cmd_char(const CommandArgs *a)
{
    const char *chars = a->args[0];
    if (chars[0] == '\0') {
        error_msg("char argument can't be empty");
        return;
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

    Condition *c = add_condition(type, a->args[1], a->args[2]);
    if (!c) {
        return;
    }

    if (type == COND_CHAR1) {
        c->u.ch = (unsigned char)chars[0];
    } else {
        bitset_add_char_range(c->u.bitset, chars);
        if (invert) {
            BITSET_INVERT(c->u.bitset);
        }
    }
}

static void cmd_default(const CommandArgs *a)
{
    close_state();
    if (no_syntax()) {
        return;
    }

    const char *value = str_intern(a->args[0]);
    HashMap *map = &current_syntax->default_colors;
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *name = a->args[i];
        void *oldval = hashmap_insert_or_replace(map, xstrdup(name), (char*)value);
        if (unlikely(oldval)) {
            DEBUG_LOG (
                "duplicate 'default' argument in %s:%d: '%s'",
                current_config.file,
                current_config.line,
                name
            );
        }
    }
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
    if (!list) {
        list = xnew0(StringList, 1);
        hashmap_insert(&current_syntax->string_lists, xstrdup(name), list);
    } else if (list->defined) {
        error_msg("List %s already exists.", name);
        return;
    }
    list->defined = true;

    bool icase = a->flags[0] == 'i';
    HashSet *set = &list->strings;
    hashset_init(set, a->nr_args - 1, icase);
    for (size_t i = 1, n = a->nr_args; i < n; i++) {
        const char *str = args[i];
        hashset_add(set, str, strlen(str));
    }
}

static void cmd_inlist(const CommandArgs *a)
{
    char **args = a->args;
    const char *name = args[0];
    const char *emit = args[2] ? args[2] : name;
    StringList *list = find_string_list(current_syntax, name);
    Condition *c = add_condition(COND_INLIST, args[1], emit);

    if (!c) {
        return;
    }

    if (!list) {
        // Add undefined list
        list = xnew0(StringList, 1);
        hashmap_insert(&current_syntax->string_lists, xstrdup(name), list);
    }
    list->used = true;
    c->u.str_list = list;
}

static void cmd_noeat(const CommandArgs *a)
{
    if (no_state()) {
        return;
    }

    const char *arg = a->args[0];
    if (streq(arg, current_state->name)) {
        error_msg("Using noeat to jump to parent state causes infinite loop");
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
        c->u.recolor_len = len;
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

    if (len > ARRAY_COUNT(c->u.str.buf)) {
        error_msg (
            "Maximum length of string is %zu bytes",
            ARRAY_COUNT(c->u.str.buf)
        );
        return;
    }

    // Strings of length 2 are very common
    if (!icase && len == 2) {
        type = COND_STR2;
    }
    c = add_condition(type, a->args[1], a->args[2]);
    if (c) {
        memcpy(c->u.str.buf, str, len);
        c->u.str.len = len;
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
static void cmd_require(const CommandArgs *a);

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

UNITTEST {
    CHECK_BSEARCH_ARRAY(cmds, name, strcmp);
}

static const Command *find_syntax_command(const char *name)
{
    return BSEARCH(name, cmds, command_cmp);
}

static const CommandSet syntax_commands = {
    .lookup = find_syntax_command,
    .allow_recording = NULL,
};

static void cmd_include(const CommandArgs *a)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (a->flags[0] == 'b') {
        flags |= CFG_BUILTIN;
    }
    read_config(&syntax_commands, a->args[0], flags);
}

static void cmd_require(const CommandArgs *a)
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
        return;
    }

    if (read_config(&syntax_commands, path, flags) == 0) {
        hashset_add(set, path, path_len);
    }
}

Syntax *load_syntax_file(const char *filename, ConfigFlags flags, int *err)
{
    const ConfigState saved = current_config;
    *err = do_read_config(&syntax_commands, filename, flags);
    if (*err) {
        current_config = saved;
        return NULL;
    }
    if (current_syntax) {
        finish_syntax();
        find_unused_subsyntaxes();
    }
    current_config = saved;

    Syntax *syn = find_syntax(path_basename(filename));
    if (syn && editor.status != EDITOR_INITIALIZING) {
        update_syntax_colors(syn);
    }
    if (!syn) {
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
