#include <dirent.h>
#include <sys/stat.h>
#include "completion.h"
#include "bind.h"
#include "command/alias.h"
#include "command/args.h"
#include "command/parse.h"
#include "command/serialize.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "filetype.h"
#include "options.h"
#include "show.h"
#include "syntax/color.h"
#include "tag.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/xmalloc.h"
#include "vars.h"

typedef struct {
    char *orig; // Full cmdline string (backing buffer for `escaped` and `tail`)
    char *parsed; // Result of passing `escaped` through parse_command_arg()
    StringView escaped; // Middle part of `orig` (string to be replaced)
    StringView tail; // Suffix part of `orig` (after `escaped`)
    size_t head_len; // Length of prefix part of `orig` (before `escaped`)

    PointerArray completions; // Array of completion candidates
    size_t idx; // Index of currently selected completion

    bool add_space_after_single_match;
    bool tilde_expanded;
} CompletionState;

static CompletionState completion;

static void do_collect_files (
    const char *dirname,
    const char *dirprefix,
    const char *fileprefix,
    bool directories_only
) {
    char path[8192];
    size_t plen = strlen(dirname);
    const size_t flen = strlen(fileprefix);
    BUG_ON(plen == 0U);

    if (plen >= sizeof(path) - 2) {
        return;
    }

    DIR *const dir = opendir(dirname);
    if (!dir) {
        return;
    }

    memcpy(path, dirname, plen);
    if (path[plen - 1] != '/') {
        path[plen++] = '/';
    }

    const struct dirent *de;
    while ((de = readdir(dir))) {
        const char *name = de->d_name;

        if (flen) {
            if (strncmp(name, fileprefix, flen) != 0) {
                continue;
            }
        } else {
            if (name[0] == '.') {
                continue;
            }
        }

        size_t len = strlen(name);
        if (plen + len + 2 > sizeof(path)) {
            continue;
        }
        memcpy(path + plen, name, len + 1);

        struct stat st;
        if (lstat(path, &st)) {
            continue;
        }

        bool is_dir = S_ISDIR(st.st_mode);
        if (S_ISLNK(st.st_mode)) {
            if (!stat(path, &st)) {
                is_dir = S_ISDIR(st.st_mode);
            }
        }
        if (!is_dir && directories_only) {
            continue;
        }

        String buf = string_new(strlen(dirprefix) + len + 4);
        if (dirprefix[0]) {
            string_append_cstring(&buf, dirprefix);
            if (!str_has_suffix(dirprefix, "/")) {
                string_append_byte(&buf, '/');
            }
        }
        string_append_cstring(&buf, name);
        if (is_dir) {
            string_append_byte(&buf, '/');
        }
        add_completion(string_steal_cstring(&buf));
    }
    closedir(dir);
}

static void collect_files(CompletionState *cs, bool directories_only)
{
    StringView e = cs->escaped;
    if (strview_has_prefix(&e, "~/")) {
        char *str = parse_command_arg(&normal_commands, e.data, e.length, false);
        const char *slash = strrchr(str, '/');
        BUG_ON(!slash);
        cs->tilde_expanded = true;
        char *dir = path_dirname(cs->parsed);
        char *dirprefix = path_dirname(str);
        do_collect_files(dir, dirprefix, slash + 1, directories_only);
        free(dirprefix);
        free(dir);
        free(str);
    } else {
        const char *slash = strrchr(cs->parsed, '/');
        if (!slash) {
            do_collect_files(".", "", cs->parsed, directories_only);
        } else {
            char *dir = path_dirname(cs->parsed);
            do_collect_files(dir, dir, slash + 1, directories_only);
            free(dir);
        }
    }

    if (cs->completions.count == 1) {
        // Add space if completed string is not a directory
        const char *s = cs->completions.ptrs[0];
        size_t len = strlen(s);
        if (len > 0) {
            cs->add_space_after_single_match = s[len - 1] != '/';
        }
    }
}

static void complete_files(CompletionState *cs, const CommandArgs* UNUSED_ARG(a))
{
    collect_files(cs, false);
}

static void complete_dirs(CompletionState *cs, const CommandArgs* UNUSED_ARG(a))
{
    collect_files(cs, true);
}

static void complete_compile(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_compilers(cs->parsed);
    }
}

static void complete_errorfmt(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_compilers(cs->parsed);
    } else if (a->nr_args >= 2 && !cmdargs_has_flag(a, 'i')) {
        collect_errorfmt_capture_names(cs->parsed);
    }
}

static void complete_ft(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_ft(cs->parsed);
    }
}

static void complete_hi(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_hl_colors(cs->parsed);
    } else {
        collect_colors_and_attributes(cs->parsed);
    }
}

static void complete_include(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        if (cmdargs_has_flag(a, 'b')) {
            collect_builtin_configs(cs->parsed);
        } else {
            collect_files(cs, false);
        }
    }
}

static void complete_macro(CompletionState *cs, const CommandArgs *a)
{
    static const char verbs[][8] = {
        "cancel",
        "play",
        "record",
        "stop",
        "toggle",
    };

    if (a->nr_args != 0) {
        return;
    }

    for (size_t i = 0; i < ARRAY_COUNT(verbs); i++) {
        if (str_has_prefix(verbs[i], cs->parsed)) {
            add_completion(xstrdup(verbs[i]));
        }
    }
}

static void complete_open(CompletionState *cs, const CommandArgs *a)
{
    if (!cmdargs_has_flag(a, 't')) {
        collect_files(cs, false);
    }
}

static void complete_option(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        if (!cmdargs_has_flag(a, 'r')) {
            collect_ft(cs->parsed);
        }
    } else if (a->nr_args & 1) {
        collect_auto_options(cs->parsed);
    } else {
        collect_option_values(a->args[a->nr_args - 1], cs->parsed);
    }
}

static void complete_set(CompletionState *cs, const CommandArgs *a)
{
    if ((a->nr_args + 1) & 1) {
        bool local = cmdargs_has_flag(a, 'l');
        bool global = cmdargs_has_flag(a, 'g');
        collect_options(cs->parsed, local, global);
    } else {
        collect_option_values(a->args[a->nr_args - 1], cs->parsed);
    }
}

static void complete_setenv(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_env(cs->parsed);
    } else if (a->nr_args == 1 && cs->parsed[0] == '\0') {
        BUG_ON(!a->args[0]);
        const char *value = getenv(a->args[0]);
        if (value) {
            add_completion(xstrdup(value));
        }
    }
}

static void complete_show(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        collect_show_subcommands(cs->parsed);
    } else if (a->nr_args == 1) {
        BUG_ON(!a->args[0]);
        collect_show_subcommand_args(a->args[0], cs->parsed);
    }
}

static void complete_tag(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0 && !cmdargs_has_flag(a, 'r')) {
        TagFile *tf = load_tag_file();
        if (tf) {
            collect_tags(tf, cs->parsed);
        }
    }
}

static void complete_toggle(CompletionState *cs, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        bool global = cmdargs_has_flag(a, 'g');
        collect_toggleable_options(cs->parsed, global);
    }
}

static void complete_wsplit(CompletionState *cs, const CommandArgs *a)
{
    if (!cmdargs_has_flag(a, 't') && !cmdargs_has_flag(a, 'n')) {
        collect_files(cs, false);
    }
}

typedef struct {
    char cmd_name[12];
    void (*complete)(CompletionState *cs, const CommandArgs *a);
} CompletionHandler;

static const CompletionHandler handlers[] = {
    {"cd", complete_dirs},
    {"compile", complete_compile},
    {"errorfmt", complete_errorfmt},
    {"ft", complete_ft},
    {"hi", complete_hi},
    {"include", complete_include},
    {"macro", complete_macro},
    {"open", complete_open},
    {"option", complete_option},
    {"pipe-from", complete_files},
    {"pipe-to", complete_files},
    {"run", complete_files},
    {"save", complete_files},
    {"set", complete_set},
    {"setenv", complete_setenv},
    {"show", complete_show},
    {"tag", complete_tag},
    {"toggle", complete_toggle},
    {"wsplit", complete_wsplit},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(handlers, cmd_name, strcmp);
}

static void collect_completions(CompletionState *cs, char **args, size_t argc)
{
    if (!argc) {
        collect_normal_commands(cs->parsed);
        collect_normal_aliases(cs->parsed);
        return;
    }

    const Command *cmd = find_normal_command(args[0]);
    if (!cmd || cmd->max_args == 0) {
        return;
    }

    char **args_copy = copy_string_array(args + 1, argc - 1);
    CommandArgs a = {.args = args_copy};
    ArgParseError err = do_parse_args(cmd, &a);
    if (
        (err != 0 && err != ARGERR_TOO_FEW_ARGUMENTS)
        || (a.nr_args >= cmd->max_args && cmd->max_args != 0xFF)
    ) {
        goto out;
    }

    const CompletionHandler *h = BSEARCH(args[0], handlers, (CompareFunction)strcmp);
    if (h) {
        h->complete(cs, &a);
    } else if (streq(args[0], "repeat")) {
        if (a.nr_args == 1) {
            collect_normal_commands(cs->parsed);
        } else if (a.nr_args >= 2) {
            collect_completions(cs, args + 2, argc - 2);
        }
    }

out:
    free_string_array(args_copy);
}

static bool is_var(const char *str, size_t len)
{
    if (len == 0 || str[0] != '$') {
        return false;
    }
    if (len == 1) {
        return true;
    }
    if (!is_alpha_or_underscore(str[1])) {
        return false;
    }
    for (size_t i = 2; i < len; i++) {
        if (!is_alnum_or_underscore(str[i])) {
            return false;
        }
    }
    return true;
}

UNITTEST {
    BUG_ON(!is_var(STRN("$VAR")));
    BUG_ON(!is_var(STRN("$xy_190")));
    BUG_ON(!is_var(STRN("$__x_y_z")));
    BUG_ON(!is_var(STRN("$x")));
    BUG_ON(!is_var(STRN("$A")));
    BUG_ON(!is_var(STRN("$_0")));
    BUG_ON(!is_var(STRN("$")));
    BUG_ON(is_var(STRN("")));
    BUG_ON(is_var(STRN("A")));
    BUG_ON(is_var(STRN("$.a")));
    BUG_ON(is_var(STRN("$xyz!")));
    BUG_ON(is_var(STRN("$1")));
    BUG_ON(is_var(STRN("$09")));
    BUG_ON(is_var(STRN("$1a")));
}

static int strptrcmp(const void *v1, const void *v2)
{
    const char *const *s1 = v1;
    const char *const *s2 = v2;
    return strcmp(*s1, *s2);
}

static void init_completion(CompletionState *cs, CommandLine *cmdline)
{
    BUG_ON(cs->orig);
    const CommandSet *cmds = &normal_commands;
    const size_t cmdline_pos = cmdline->pos;
    char *const cmd = string_clone_cstring(&cmdline->buf);
    PointerArray array = PTR_ARRAY_INIT;
    ssize_t semicolon = -1;
    ssize_t completion_pos = -1;

    for (size_t pos = 0; true; ) {
        while (ascii_isspace(cmd[pos])) {
            pos++;
        }

        if (pos >= cmdline_pos) {
            completion_pos = cmdline_pos;
            break;
        }

        if (!cmd[pos]) {
            break;
        }

        if (cmd[pos] == ';') {
            semicolon = array.count;
            ptr_array_append(&array, NULL);
            pos++;
            continue;
        }

        CommandParseError err;
        size_t end = find_end(cmd, pos, &err);
        if (err != CMDERR_NONE || end >= cmdline_pos) {
            completion_pos = pos;
            break;
        }

        if (semicolon + 1 == array.count) {
            char *name = xstrslice(cmd, pos, end);
            const char *value = find_alias(&normal_commands.aliases, name);
            if (value) {
                size_t save = array.count;
                if (parse_commands(cmds, &array, value) != CMDERR_NONE) {
                    for (size_t i = save, n = array.count; i < n; i++) {
                        free(array.ptrs[i]);
                        array.ptrs[i] = NULL;
                    }
                    array.count = save;
                    ptr_array_append(&array, parse_command_arg(cmds, name, end - pos, true));
                } else {
                    // Remove NULL
                    array.count--;
                }
            } else {
                ptr_array_append(&array, parse_command_arg(cmds, name, end - pos, true));
            }
            free(name);
        } else {
            ptr_array_append(&array, parse_command_arg(cmds, cmd + pos, end - pos, true));
        }
        pos = end;
    }

    const char *str = cmd + completion_pos;
    size_t len = cmdline_pos - completion_pos;
    if (is_var(str, len)) {
        char *name = xstrslice(str, 1, len);
        completion_pos++;
        collect_env(name);
        collect_normal_vars(name);
        free(name);
    } else {
        cs->escaped = string_view(str, len);
        cs->parsed = parse_command_arg(cmds, str, len, true);
        cs->add_space_after_single_match = true;
        char **args = NULL;
        size_t argc = 0;
        if (array.count) {
            args = (char**)array.ptrs + 1 + semicolon;
            argc = array.count - semicolon - 1;
        }
        collect_completions(cs, args, argc);
    }

    ptr_array_free(&array);
    ptr_array_sort(&cs->completions, strptrcmp);
    cs->orig = cmd; // (takes ownership)
    cs->tail = strview_from_cstring(cmd + cmdline_pos);
    cs->head_len = completion_pos;
}

static void do_complete_command(CompletionState *cs, CommandLine *cmdline)
{
    const PointerArray *arr = &cs->completions;
    const StringView middle = strview_from_cstring(arr->ptrs[cs->idx]);
    const StringView tail = cs->tail;
    const size_t head_length = cs->head_len;

    String buf = string_new(head_length + tail.length + middle.length + 16);
    string_append_buf(&buf, cs->orig, head_length);
    string_append_escaped_arg_sv(&buf, middle, !cs->tilde_expanded);

    bool single_completion = (arr->count == 1);
    if (single_completion && cs->add_space_after_single_match) {
        string_append_byte(&buf, ' ');
    }

    size_t pos = buf.len;
    string_append_strview(&buf, &tail);
    cmdline_set_text(cmdline, string_borrow_cstring(&buf));
    cmdline->pos = pos;
    string_free(&buf);

    if (single_completion) {
        reset_completion();
    }
}

void complete_command_next(CommandLine *cmdline)
{
    CompletionState *cs = &completion;
    const bool init = !cs->orig;
    if (init) {
        init_completion(cs, cmdline);
    }
    if (!cs->completions.count) {
        return;
    }
    if (!init) {
        if (cs->idx >= cs->completions.count - 1) {
            cs->idx = 0;
        } else {
            cs->idx++;
        }
    }
    do_complete_command(cs, cmdline);
}

void complete_command_prev(CommandLine *cmdline)
{
    CompletionState *cs = &completion;
    const bool init = !cs->orig;
    if (init) {
        init_completion(cs, cmdline);
    }
    if (!cs->completions.count) {
        return;
    }
    if (!init) {
        if (cs->idx == 0) {
            cs->idx = cs->completions.count - 1;
        } else {
            cs->idx--;
        }
    }
    do_complete_command(cs, cmdline);
}

void reset_completion(void)
{
    CompletionState *cs = &completion;
    free(cs->parsed);
    free(cs->orig);
    ptr_array_free(&cs->completions);
    MEMZERO(cs);
}

void add_completion(char *str)
{
    ptr_array_append(&completion.completions, str);
}

void collect_hashmap_keys(const HashMap *map, const char *prefix)
{
    for (HashMapIter it = hashmap_iter(map); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        if (str_has_prefix(name, prefix)) {
            add_completion(xstrdup(name));
        }
    }
}
