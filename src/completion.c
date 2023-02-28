#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "completion.h"
#include "bind.h"
#include "command/alias.h"
#include "command/args.h"
#include "command/parse.h"
#include "command/run.h"
#include "command/serialize.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "filetype.h"
#include "options.h"
#include "show.h"
#include "syntax/color.h"
#include "tag.h"
#include "terminal/cursor.h"
#include "terminal/key.h"
#include "terminal/style.h"
#include "util/arith.h"
#include "util/array.h"
#include "util/ascii.h"
#include "util/bsearch.h"
#include "util/debug.h"
#include "util/intmap.h"
#include "util/log.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/xdirent.h"
#include "util/xmalloc.h"
#include "vars.h"

extern char **environ;

typedef enum {
    COLLECT_ALL, // (directories and files)
    COLLECT_EXECUTABLES, // (directories and executable files)
    COLLECT_DIRS_ONLY,
} FileCollectionType;

static bool is_executable(int dir_fd, const char *filename)
{
    return faccessat(dir_fd, filename, X_OK, 0) == 0;
}

static bool do_collect_files (
    PointerArray *array,
    const char *dirname,
    const char *dirprefix,
    const char *fileprefix,
    FileCollectionType type
) {
    DIR *const dir = xopendir(dirname);
    if (!dir) {
        return false;
    }

    const int dir_fd = dirfd(dir);
    if (unlikely(dir_fd < 0)) {
        LOG_ERRNO("dirfd");
        xclosedir(dir);
        return false;
    }

    size_t dlen = strlen(dirprefix);
    size_t flen = strlen(fileprefix);
    const struct dirent *de;

    while ((de = xreaddir(dir))) {
        const char *name = de->d_name;
        if (streq(name, ".") || streq(name, "..") || unlikely(streq(name, ""))) {
            continue;
        }

        // TODO: add a global option to allow dotfiles to be included
        // even when there's no prefix
        if (flen ? strncmp(name, fileprefix, flen) : name[0] == '.') {
            continue;
        }

        struct stat st;
        if (fstatat(dir_fd, name, &st, AT_SYMLINK_NOFOLLOW)) {
            continue;
        }

        bool is_dir = S_ISDIR(st.st_mode);
        if (S_ISLNK(st.st_mode)) {
            if (!fstatat(dir_fd, name, &st, 0)) {
                is_dir = S_ISDIR(st.st_mode);
            }
        }

        if (!is_dir) {
            switch (type) {
            case COLLECT_DIRS_ONLY:
                continue;
            case COLLECT_ALL:
                break;
            case COLLECT_EXECUTABLES:
                if (!is_executable(dir_fd, name)) {
                    continue;
                }
                if (!dlen) {
                    dirprefix = "./";
                    dlen = 2;
                }
                break;
            default:
                BUG("unhandled FileCollectionType value");
            }
        }

        ptr_array_append(array, path_joinx(dirprefix, name, is_dir));
    }

    xclosedir(dir);
    return true;
}

static void collect_files(EditorState *e, CompletionState *cs, FileCollectionType type)
{
    StringView esc = cs->escaped;
    if (strview_has_prefix(&esc, "~/")) {
        CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
        char *str = parse_command_arg(&runner, esc.data, esc.length, false);
        const char *slash = strrchr(str, '/');
        BUG_ON(!slash);
        cs->tilde_expanded = true;
        char *dir = path_dirname(cs->parsed);
        char *dirprefix = path_dirname(str);
        do_collect_files(&cs->completions, dir, dirprefix, slash + 1, type);
        free(dirprefix);
        free(dir);
        free(str);
    } else {
        const char *slash = strrchr(cs->parsed, '/');
        if (!slash) {
            do_collect_files(&cs->completions, ".", "", cs->parsed, type);
        } else {
            char *dir = path_dirname(cs->parsed);
            do_collect_files(&cs->completions, dir, dir, slash + 1, type);
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

void collect_normal_aliases(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_hashmap_keys(&e->aliases, a, prefix);
}

static void collect_bound_keys(const IntMap *bindings, PointerArray *a, const char *prefix)
{
    char keystr[KEYCODE_STR_MAX];
    for (IntMapIter it = intmap_iter(bindings); intmap_next(&it); ) {
        size_t keylen = keycode_to_string(it.entry->key, keystr);
        if (str_has_prefix(keystr, prefix)) {
            ptr_array_append(a, xmemdup(keystr, keylen + 1));
        }
    }
}

void collect_bound_normal_keys(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_bound_keys(&e->modes[INPUT_NORMAL].key_bindings, a, prefix);
}

void collect_hl_colors(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_builtin_colors(a, prefix);
    collect_hashmap_keys(&e->colors.other, a, prefix);
}

void collect_compilers(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_hashmap_keys(&e->compilers, a, prefix);
}

void collect_env(EditorState* UNUSED_ARG(e), PointerArray *a, const char *prefix)
{
    if (strchr(prefix, '=')) {
        return;
    }

    for (size_t i = 0; environ[i]; i++) {
        const char *var = environ[i];
        if (str_has_prefix(var, prefix)) {
            const char *delim = strchr(var, '=');
            if (likely(delim && delim != var)) {
                ptr_array_append(a, xstrcut(var, delim - var));
            }
        }
    }
}

static void complete_alias(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_normal_aliases(e, &cs->completions, cs->parsed);
    } else if (a->nr_args == 1 && cs->parsed[0] == '\0') {
        const char *cmd = find_alias(&e->aliases, a->args[0]);
        if (cmd) {
            ptr_array_append(&cs->completions, xstrdup(cmd));
        }
    }
}

static void complete_bind(EditorState *e, const CommandArgs *a)
{
    static const char flags[] = {
        [INPUT_NORMAL] = 'n',
        [INPUT_COMMAND] = 'c',
        [INPUT_SEARCH] = 's',
    };

    static_assert(ARRAYLEN(flags) == ARRAYLEN(e->modes));
    InputMode mode = INPUT_NORMAL;
    for (size_t i = 0, count = 0; i < ARRAYLEN(flags); i++) {
        if (cmdargs_has_flag(a, flags[i])) {
            if (++count >= 2) {
                return; // Don't complete bindings for multiple modes
            }
            mode = i;
        }
    }

    const IntMap *key_bindings = &e->modes[mode].key_bindings;
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_bound_keys(key_bindings, &cs->completions, cs->parsed);
        return;
    }

    if (a->nr_args != 1 || cs->parsed[0] != '\0') {
        return;
    }

    KeyCode key;
    if (!parse_key_string(&key, a->args[0])) {
        return;
    }
    const CachedCommand *cmd = lookup_binding(key_bindings, key);
    if (!cmd) {
        return;
    }

    ptr_array_append(&cs->completions, xstrdup(cmd->cmd_str));
}

static void complete_cd(EditorState *e, const CommandArgs* UNUSED_ARG(a))
{
    CompletionState *cs = &e->cmdline.completion;
    collect_files(e, cs, COLLECT_DIRS_ONLY);
    if (str_has_prefix("-", cs->parsed)) {
        if (likely(xgetenv("OLDPWD"))) {
            ptr_array_append(&cs->completions, xstrdup("-"));
        }
    }
}

static void complete_exec(EditorState *e, const CommandArgs *a)
{
    // TODO: add completion for [-ioe] option arguments
    CompletionState *cs = &e->cmdline.completion;
    collect_files(e, cs, a->nr_args == 0 ? COLLECT_EXECUTABLES : COLLECT_ALL);
}

static void complete_compile(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    size_t n = a->nr_args;
    if (n == 0) {
        collect_compilers(e, &cs->completions, cs->parsed);
    } else {
        collect_files(e, cs, n == 1 ? COLLECT_EXECUTABLES : COLLECT_ALL);
    }
}

static void complete_cursor(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    size_t n = a->nr_args;
    if (n == 0) {
        collect_cursor_modes(&cs->completions, cs->parsed);
    } else if (n == 1) {
        collect_cursor_types(&cs->completions, cs->parsed);
    } else if (n == 2) {
        collect_cursor_colors(&cs->completions, cs->parsed);
        // Add an example #rrggbb color, to make things more discoverable
        static const char rgb_example[] = "#22AABB";
        if (str_has_prefix(rgb_example, cs->parsed)) {
            ptr_array_append(&cs->completions, xstrdup(rgb_example));
        }
    }
}

static void complete_errorfmt(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_compilers(e, &cs->completions, cs->parsed);
    } else if (a->nr_args >= 2 && !cmdargs_has_flag(a, 'i')) {
        collect_errorfmt_capture_names(&cs->completions, cs->parsed);
    }
}

static void complete_ft(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_ft(&e->filetypes, &cs->completions, cs->parsed);
    }
}

static void complete_hi(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_hl_colors(e, &cs->completions, cs->parsed);
    } else {
        collect_colors_and_attributes(&cs->completions, cs->parsed);
    }
}

static void complete_include(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        if (cmdargs_has_flag(a, 'b')) {
            collect_builtin_includes(&cs->completions, cs->parsed);
        } else {
            collect_files(e, cs, COLLECT_ALL);
        }
    }
}

static void complete_macro(EditorState *e, const CommandArgs *a)
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

    CompletionState *cs = &e->cmdline.completion;
    COLLECT_STRINGS(verbs, &cs->completions, cs->parsed);
}

static void complete_move_tab(EditorState *e, const CommandArgs *a)
{
    if (a->nr_args != 0) {
        return;
    }

    static const char words[][8] = {"left", "right"};
    CompletionState *cs = &e->cmdline.completion;
    COLLECT_STRINGS(words, &cs->completions, cs->parsed);
}

static void complete_open(EditorState *e, const CommandArgs *a)
{
    if (!cmdargs_has_flag(a, 't')) {
        collect_files(e, &e->cmdline.completion, COLLECT_ALL);
    }
}

static void complete_option(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        if (!cmdargs_has_flag(a, 'r')) {
            collect_ft(&e->filetypes, &cs->completions, cs->parsed);
        }
    } else if (a->nr_args & 1) {
        collect_auto_options(&cs->completions, cs->parsed);
    } else {
        collect_option_values(e, &cs->completions, a->args[a->nr_args - 1], cs->parsed);
    }
}

static void complete_save(EditorState *e, const CommandArgs* UNUSED_ARG(a))
{
    collect_files(e, &e->cmdline.completion, COLLECT_ALL);
}

static void complete_quit(EditorState *e, const CommandArgs* UNUSED_ARG(a))
{
    CompletionState *cs = &e->cmdline.completion;
    if (str_has_prefix("0", cs->parsed)) {
        ptr_array_append(&cs->completions, xstrdup("0"));
    }
    if (str_has_prefix("1", cs->parsed)) {
        ptr_array_append(&cs->completions, xstrdup("1"));
    }
}

static void complete_redo(EditorState *e, const CommandArgs* UNUSED_ARG(a))
{
    const Change *change = e->buffer->cur_change;
    CompletionState *cs = &e->cmdline.completion;
    for (unsigned long i = 1, n = change->nr_prev; i <= n; i++) {
        ptr_array_append(&cs->completions, xstrdup(ulong_to_str(i)));
    }
}

static void complete_set(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if ((a->nr_args + 1) & 1) {
        bool local = cmdargs_has_flag(a, 'l');
        bool global = cmdargs_has_flag(a, 'g');
        collect_options(&cs->completions, cs->parsed, local, global);
    } else {
        collect_option_values(e, &cs->completions, a->args[a->nr_args - 1], cs->parsed);
    }
}

static void complete_setenv(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_env(e, &cs->completions, cs->parsed);
    } else if (a->nr_args == 1 && cs->parsed[0] == '\0') {
        BUG_ON(!a->args[0]);
        const char *value = getenv(a->args[0]);
        if (value) {
            ptr_array_append(&cs->completions, xstrdup(value));
        }
    }
}

static void complete_show(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_show_subcommands(&cs->completions, cs->parsed);
    } else if (a->nr_args == 1) {
        BUG_ON(!a->args[0]);
        collect_show_subcommand_args(e, &cs->completions, a->args[0], cs->parsed);
    }
}

static void complete_tag(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0 && !cmdargs_has_flag(a, 'r')) {
        BUG_ON(!cs->parsed);
        StringView prefix = strview_from_cstring(cs->parsed);
        collect_tags(&e->tagfile, &cs->completions, &prefix);
    }
}

static void complete_toggle(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        bool global = cmdargs_has_flag(a, 'g');
        collect_toggleable_options(&cs->completions, cs->parsed, global);
    }
}

static void complete_wsplit(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    if (!cmdargs_has_flag(a, 't') && !cmdargs_has_flag(a, 'n')) {
        collect_files(e, cs, COLLECT_ALL);
    }
}

typedef struct {
    char cmd_name[12];
    void (*complete)(EditorState *e, const CommandArgs *a);
} CompletionHandler;

static const CompletionHandler completion_handlers[] = {
    {"alias", complete_alias},
    {"bind", complete_bind},
    {"cd", complete_cd},
    {"compile", complete_compile},
    {"cursor", complete_cursor},
    {"errorfmt", complete_errorfmt},
    {"exec", complete_exec},
    {"ft", complete_ft},
    {"hi", complete_hi},
    {"include", complete_include},
    {"macro", complete_macro},
    {"move-tab", complete_move_tab},
    {"open", complete_open},
    {"option", complete_option},
    {"quit", complete_quit},
    {"redo", complete_redo},
    {"save", complete_save},
    {"set", complete_set},
    {"setenv", complete_setenv},
    {"show", complete_show},
    {"tag", complete_tag},
    {"toggle", complete_toggle},
    {"wsplit", complete_wsplit},
};

UNITTEST {
    CHECK_BSEARCH_ARRAY(completion_handlers, cmd_name, strcmp);
    // Ensure handlers are kept in sync with renamed/removed commands
    for (size_t i = 0; i < ARRAYLEN(completion_handlers); i++) {
        const char *name = completion_handlers[i].cmd_name;
        if (!find_normal_command(name)) {
            BUG("completion handler for non-existent command: \"%s\"", name);
        }
    }
}

static bool can_collect_flags (
    char **args,
    size_t argc,
    size_t nr_flag_args,
    bool allow_flags_after_nonflags
) {
    if (allow_flags_after_nonflags) {
        for (size_t i = 0; i < argc; i++) {
            if (streq(args[i], "--")) {
                return false;
            }
        }
        return true;
    }

    for (size_t i = 0, nonflag = 0; i < argc; i++) {
        if (args[i][0] != '-') {
            if (++nonflag > nr_flag_args) {
                return false;
            }
            continue;
        }
        if (streq(args[i], "--")) {
            return false;
        }
    }

    return true;
}

static bool collect_command_flags (
    PointerArray *array,
    char **args,
    size_t argc,
    const Command *cmd,
    const CommandArgs *a,
    const char *prefix
) {
    BUG_ON(prefix[0] != '-');
    const char *flags = cmd->flags;
    bool flags_after_nonflags = (flags[0] != '-');

    if (!can_collect_flags(args, argc, a->nr_flag_args, flags_after_nonflags)) {
        return false;
    }

    flags += flags_after_nonflags ? 0 : 1;
    if (ascii_isalnum(prefix[1]) && prefix[2] == '\0') {
        if (strchr(flags, prefix[1])) {
            ptr_array_append(array, xmemdup(prefix, 3));
        }
        return true;
    }

    if (prefix[1] != '\0') {
        return true;
    }

    char buf[3] = "-";
    for (size_t i = 0; flags[i]; i++) {
        if (!ascii_isalnum(flags[i]) || cmdargs_has_flag(a, flags[i])) {
            continue;
        }
        buf[1] = flags[i];
        ptr_array_append(array, xmemdup(buf, 3));
    }

    return true;
}

static void collect_completions(EditorState *e, char **args, size_t argc)
{
    CompletionState *cs = &e->cmdline.completion;
    PointerArray *arr = &cs->completions;
    const char *prefix = cs->parsed;
    if (!argc) {
        collect_normal_commands(arr, prefix);
        collect_normal_aliases(e, arr, prefix);
        return;
    }

    for (size_t i = 0; i < argc; i++) {
        if (!args[i]) {
            // Embedded NULLs indicate there are multiple commands.
            // Just return early here and avoid handling this case.
            return;
        }
    }

    const Command *cmd = find_normal_command(args[0]);
    if (!cmd) {
        return;
    }

    char **args_copy = copy_string_array(args + 1, argc - 1);
    CommandArgs a = cmdargs_new(args_copy);
    ArgParseError err = do_parse_args(cmd, &a);
    bool dash = (prefix[0] == '-');
    if (
        (err != ARGERR_NONE && err != ARGERR_TOO_FEW_ARGUMENTS)
        || (a.nr_args >= cmd->max_args && cmd->max_args != 0xFF && !dash)
    ) {
        goto out;
    }

    if (dash && collect_command_flags(arr, args + 1, argc - 1, cmd, &a, prefix)) {
        goto out;
    }

    if (cmd->max_args == 0) {
        goto out;
    }

    const CompletionHandler *h = BSEARCH(args[0], completion_handlers, vstrcmp);
    if (h) {
        h->complete(e, &a);
    } else if (streq(args[0], "repeat")) {
        if (a.nr_args == 1) {
            collect_normal_commands(arr, prefix);
        } else if (a.nr_args >= 2) {
            collect_completions(e, args + 2, argc - 2);
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

static void init_completion(EditorState *e, const CommandLine *cmdline)
{
    CompletionState *cs = &e->cmdline.completion;
    const CommandRunner runner = cmdrunner_for_mode(e, INPUT_NORMAL, false);
    BUG_ON(cs->orig);
    BUG_ON(runner.userdata != e);
    BUG_ON(!runner.lookup_alias);

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
            const char *value = runner.lookup_alias(name, runner.userdata);
            if (value) {
                size_t save = array.count;
                if (parse_commands(&runner, &array, value) != CMDERR_NONE) {
                    for (size_t i = save, n = array.count; i < n; i++) {
                        free(array.ptrs[i]);
                        array.ptrs[i] = NULL;
                    }
                    array.count = save;
                    ptr_array_append(&array, parse_command_arg(&runner, name, end - pos, true));
                } else {
                    // Remove NULL
                    array.count--;
                }
            } else {
                ptr_array_append(&array, parse_command_arg(&runner, name, end - pos, true));
            }
            free(name);
        } else {
            ptr_array_append(&array, parse_command_arg(&runner, cmd + pos, end - pos, true));
        }
        pos = end;
    }

    const char *str = cmd + completion_pos;
    size_t len = cmdline_pos - completion_pos;
    if (is_var(str, len)) {
        char *name = xstrslice(str, 1, len);
        completion_pos++;
        collect_env(e, &cs->completions, name);
        collect_normal_vars(&cs->completions, name);
        free(name);
    } else {
        cs->escaped = string_view(str, len);
        cs->parsed = parse_command_arg(&runner, str, len, true);
        cs->add_space_after_single_match = true;
        size_t count = array.count;
        char **args = count ? (char**)array.ptrs + 1 + semicolon : NULL;
        size_t argc = count ? array.count - semicolon - 1 : 0;
        collect_completions(e, args, argc);
    }

    ptr_array_free(&array);
    ptr_array_sort(&cs->completions, strptrcmp);
    cs->orig = cmd; // (takes ownership)
    cs->tail = strview_from_cstring(cmd + cmdline_pos);
    cs->head_len = completion_pos;
}

static void do_complete_command(CommandLine *cmdline)
{
    const CompletionState *cs = &cmdline->completion;
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
        reset_completion(cmdline);
    }
}

void complete_command_next(EditorState *e)
{
    CompletionState *cs = &e->cmdline.completion;
    const bool init = !cs->orig;
    if (init) {
        init_completion(e, &e->cmdline);
    }
    size_t count = cs->completions.count;
    if (!count) {
        return;
    }
    if (!init) {
        cs->idx = size_increment_wrapped(cs->idx, count);
    }
    do_complete_command(&e->cmdline);
}

void complete_command_prev(EditorState *e)
{
    CompletionState *cs = &e->cmdline.completion;
    const bool init = !cs->orig;
    if (init) {
        init_completion(e, &e->cmdline);
    }
    size_t count = cs->completions.count;
    if (!count) {
        return;
    }
    if (!init) {
        cs->idx = size_decrement_wrapped(cs->idx, count);
    }
    do_complete_command(&e->cmdline);
}

void reset_completion(CommandLine *cmdline)
{
    CompletionState *cs = &cmdline->completion;
    free(cs->parsed);
    free(cs->orig);
    ptr_array_free(&cs->completions);
    *cs = (CompletionState){.orig = NULL};
}

void collect_hashmap_keys(const HashMap *map, PointerArray *a, const char *prefix)
{
    for (HashMapIter it = hashmap_iter(map); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        if (str_has_prefix(name, prefix)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}
