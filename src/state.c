#include "state.h"
#include "syntax.h"
#include "color.h"
#include "command.h"
#include "editor.h"
#include "parse-args.h"
#include "config.h"
#include "error.h"
#include "path.h"
#include "common.h"

static void bitmap_set(unsigned char *bitmap, long idx)
{
    unsigned int byte = idx / 8;
    unsigned int bit = idx & 7;
    bitmap[byte] |= 1 << bit;
}

static void set_bits(unsigned char *bitmap, const unsigned char *pattern)
{
    for (size_t i = 0; pattern[i]; i++) {
        unsigned int ch = pattern[i];
        bitmap_set(bitmap, ch);
        if (pattern[i + 1] == '-' && pattern[i + 2]) {
            for (ch = ch + 1; ch <= pattern[i + 2]; ch++) {
                bitmap_set(bitmap, ch);
            }
            i += 2;
        }
    }
}

static Syntax *current_syntax;
static State *current_state;
static int saved_nr_errors; // Used to check if nr_errors changed

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
        char *sub = xstrslice(name, 0, sep - name);
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
    Condition *c;
    State *d = NULL;

    if (no_state()) {
        return NULL;
    }

    if (dest && !destination_state(dest, &d)) {
        return NULL;
    }

    c = xnew0(Condition, 1);
    c->a.destination = d;
    c->a.emit_name = emit ? xstrdup(emit) : NULL;
    c->type = type;
    ptr_array_add(&current_state->conds, c);
    return c;
}

static void cmd_bufis(const char *pf, char **args)
{
    const bool icase = !!*pf;
    Condition *c;
    const char *str = args[0];
    const size_t len = strlen(str);

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

static void cmd_char(const char *pf, char **args)
{
    ConditionType type = COND_CHAR;
    bool not = false;
    Condition *c;

    while (*pf) {
        switch (*pf) {
        case 'b':
            type = COND_CHAR_BUFFER;
            break;
        case 'n':
            not = true;
            break;
        }
        pf++;
    }

    c = add_condition(type, args[1], args[2]);
    if (!c) {
        return;
    }

    set_bits(c->u.cond_char.bitmap, args[0]);
    if (not) {
        for (size_t i = 0; i < ARRAY_COUNT(c->u.cond_char.bitmap); i++) {
            c->u.cond_char.bitmap[i] = ~c->u.cond_char.bitmap[i];
        }
    }
}

static void cmd_default(const char* UNUSED(pf), char **args)
{
    close_state();
    if (no_syntax()) {
        return;
    }
    ptr_array_add (
        &current_syntax->default_colors,
        copy_string_array(args, count_strings(args))
    );
}

static void cmd_eat(const char* UNUSED(pf), char **args)
{
    if (no_state()) {
        return;
    }

    if (!destination_state(args[0], &current_state->a.destination)) {
        return;
    }

    current_state->type = STATE_EAT;
    current_state->a.emit_name = args[1] ? xstrdup(args[1]) : NULL;
    current_state = NULL;
}

static void cmd_heredocbegin(const char* UNUSED(pf), char **args)
{
    const char *sub;
    Syntax *subsyn;

    if (no_state()) {
        return;
    }

    sub = args[0];
    subsyn = find_any_syntax(sub);
    if (!subsyn) {
        error_msg("No such syntax %s", sub);
        return;
    }
    if (!is_subsyntax(subsyn)) {
        error_msg("Syntax %s is not subsyntax", sub);
        return;
    }

    // a.destination is used as the return state
    if (!destination_state(args[1], &current_state->a.destination)) {
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

static void cmd_heredocend(const char* UNUSED(pf), char **args)
{
    add_condition(COND_HEREDOCEND, args[0], args[1]);
    current_syntax->heredoc = true;
}

static void cmd_list(const char *pf, char **args)
{
    const char *name = args[0];
    StringList *list;

    close_state();
    if (no_syntax()) {
        return;
    }

    list = find_string_list(current_syntax, name);
    if (list == NULL) {
        list = xnew0(StringList, 1);
        list->name = xstrdup(name);
        ptr_array_add(&current_syntax->string_lists, list);
    } else if (list->defined) {
        error_msg("List %s already exists.", name);
        return;
    }
    list->defined = true;
    list->icase = !!*pf;

    for (size_t i = 1; args[i]; i++) {
        const char *str = args[i];
        const size_t len = strlen(str);
        unsigned long idx = buf_hash(str, len) % ARRAY_COUNT(list->hash);
        HashStr *h = xmalloc(sizeof(HashStr *) + sizeof(int) + len);
        h->next = list->hash[idx];
        h->len = len;
        memcpy(h->str, str, len);
        list->hash[idx] = h;
    }
}

static void cmd_inlist(const char* UNUSED(pf), char **args)
{
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

static void cmd_noeat(const char *pf, char **args)
{
    int type = *pf ? STATE_NOEAT_BUFFER : STATE_NOEAT;

    if (no_state()) {
        return;
    }

    if (streq(args[0], current_state->name)) {
        error_msg (
            "Using noeat to to jump to parent state causes"
            " infinite loop"
        );
        return;
    }

    if (!destination_state(args[0], &current_state->a.destination)) {
        return;
    }

    current_state->a.emit_name = NULL;
    current_state->type = type;
    current_state = NULL;
}

static void cmd_recolor(const char* UNUSED(pf), char **args)
{
    // If length is not specified then buffered bytes will be recolored
    ConditionType type = COND_RECOLOR_BUFFER;
    Condition *c;
    int len = 0;

    if (args[1]) {
        type = COND_RECOLOR;
        len = atoi(args[1]);
        if (len <= 0) {
            error_msg("number of bytes must be larger than 0");
            return;
        }
        if (len > 2500) {
            error_msg("number of bytes cannot be larger than 2500");
            return;
        }
    }
    c = add_condition(type, NULL, args[0]);
    if (c && type == COND_RECOLOR) {
        c->u.cond_recolor.len = (size_t)len;
    }
}

static void cmd_state(const char* UNUSED(pf), char **args)
{
    const char *name = args[0];
    const char *emit = args[1] ? args[1] : args[0];
    State *s;

    close_state();
    if (no_syntax()) {
        return;
    }
    if (streq(name, "END") || streq(name, "this")) {
        error_msg("%s is reserved state name", name);
        return;
    }
    s = find_or_add_state(name);
    if (s->defined) {
        error_msg("State %s already exists.", name);
        return;
    }
    s->defined = true;
    s->emit_name = xstrdup(emit);
    current_state = s;
}

static void cmd_str(const char *pf, char **args)
{
    bool icase = !!*pf;
    ConditionType type = icase ? COND_STR_ICASE : COND_STR;
    const char *str = args[0];
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
    c = add_condition(type, args[1], args[2]);
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

static void cmd_syntax(const char* UNUSED(pf), char **args)
{
    if (current_syntax) {
        finish_syntax();
    }

    current_syntax = xnew0(Syntax, 1);
    current_syntax->name = xstrdup(args[0]);
    current_state = NULL;

    saved_nr_errors = nr_errors;
}

static void cmd_include(const char *pf, char **args);

static const Command syntax_commands[] = {
    {"bufis", "i", 2,  3, cmd_bufis},
    {"char", "bn", 2,  3, cmd_char},
    {"default", "", 2, -1, cmd_default},
    {"eat", "", 1,  2, cmd_eat},
    {"heredocbegin","", 2,  2, cmd_heredocbegin},
    {"heredocend", "", 1,  2, cmd_heredocend},
    {"include", "b", 1,  1, cmd_include},
    {"inlist", "", 2,  3, cmd_inlist},
    {"list", "i", 2, -1, cmd_list},
    {"noeat", "b", 1,  1, cmd_noeat},
    {"recolor", "", 1,  2, cmd_recolor},
    {"state", "", 1,  2, cmd_state},
    {"str", "i", 2,  3, cmd_str},
    {"syntax", "", 1,  1, cmd_syntax},
    {NULL, NULL, 0,  0, NULL}
};

static void cmd_include(const char *pf, char **args)
{
    ConfigFlags flags = CFG_MUST_EXIST;
    if (*pf == 'b') {
        flags |= CFG_BUILTIN;
    }
    read_config(syntax_commands, args[0], flags);
}

Syntax *load_syntax_file(const char *filename, ConfigFlags flags, int *err)
{
    const char *saved_config_file = config_file;
    int saved_config_line = config_line;
    Syntax *syn;

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

    syn = find_syntax(path_basename(filename));
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
    Syntax *syn;
    char *filename = xsprintf("%s/syntax/%s", editor.user_config_dir, filetype);
    int err;

    syn = load_syntax_file(filename, CFG_NOFLAGS, &err);
    free(filename);
    if (syn || err != ENOENT) {
        return syn;
    }

    filename = xsprintf("syntax/%s", filetype);
    syn = load_syntax_file(filename, CFG_BUILTIN, &err);
    free(filename);
    return syn;
}
