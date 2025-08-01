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
#include "exec.h"
#include "filetype.h"
#include "mode.h"
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
#include "util/bit.h"
#include "util/bsearch.h"
#include "util/environ.h"
#include "util/intmap.h"
#include "util/log.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/str-array.h"
#include "util/str-util.h"
#include "util/string-view.h"
#include "util/string.h"
#include "util/xdirent.h"
#include "util/xmalloc.h"
#include "util/xstring.h"
#include "vars.h"

typedef enum {
    COLLECT_ALL, // (directories and files)
    COLLECT_EXECUTABLES, // (directories and executable files)
    COLLECT_DIRS_ONLY,
} FileCollectionType;

static bool is_executable(int dir_fd, const char *filename)
{
    return faccessat(dir_fd, filename, X_OK, 0) == 0;
}

static bool is_ignored_dir_entry(const StringView *name)
{
    return unlikely(name->length == 0)
        || strview_equal_cstring(name, ".")
        || strview_equal_cstring(name, "..");
}

static bool do_collect_files (
    PointerArray *array,
    const char *dirname,
    StringView dirprefix,
    StringView fileprefix,
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

    if (type == COLLECT_EXECUTABLES && dirprefix.length == 0) {
        dirprefix = strview("./");
    }

    for (const struct dirent *de; (de = xreaddir(dir)); ) {
        const char *name = de->d_name;
        const StringView name_sv = strview(name);
        bool has_prefix = strview_has_sv_prefix(name_sv, fileprefix);
        bool match = fileprefix.length ? has_prefix : name[0] != '.';
        if (!match || is_ignored_dir_entry(&name_sv)) {
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
                break;
            default:
                BUG("unhandled FileCollectionType value");
            }
        }

        char *path = path_join_sv(dirprefix, name_sv, is_dir);
        ptr_array_append(array, path);
    }

    xclosedir(dir);
    return true;
}

static void collect_files(EditorState *e, CompletionState *cs, FileCollectionType type)
{
    char *dir = path_dirname(cs->parsed);
    StringView dirprefix;
    StringView fileprefix;
    char buf[8192];

    if (strview_has_prefix(&cs->escaped, "~/")) {
        const StringView *home = &e->home_dir;
        if (unlikely(!strview_has_prefix(home, "/"))) {
            return;
        }

        StringView parsed = strview(cs->parsed);
        bool p = strview_remove_matching_sv_prefix(&parsed, *home);
        p = p && strview_remove_matching_prefix(&parsed, "/");
        if (unlikely(!p || sizeof(buf) < parsed.length + 3)) {
            LOG_ERROR("%s", p ? "no buffer space" : "unexpected prefix");
            return;
        }

        // Copy `cs->parsed` into `buf[]`, but with the $HOME/ prefix
        // replaced with ~/
        xmempcpy2(buf, STRN("~/"), parsed.data, parsed.length + 1);

        dirprefix = path_slice_dirname(buf);
        fileprefix = strview(buf + dirprefix.length + 1);
        cs->tilde_expanded = true;
    } else {
        const char *base = path_basename(cs->parsed);
        bool has_slash = (base != cs->parsed);
        dirprefix = strview(has_slash ? dir : "");
        fileprefix = strview(base);
    }

    do_collect_files(&cs->completions, dir, dirprefix, fileprefix, type);
    free(dir);

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
    size_t prefix_len = strlen(prefix);
    char keystr[KEYCODE_STR_BUFSIZE];
    for (IntMapIter it = intmap_iter(bindings); intmap_next(&it); ) {
        size_t keylen = keycode_to_str(it.entry->key, keystr);
        if (str_has_strn_prefix(keystr, prefix, prefix_len)) {
            ptr_array_append(a, xmemdup(keystr, keylen + 1));
        }
    }
}

void collect_bound_normal_keys(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_bound_keys(&e->normal_mode->key_bindings, a, prefix);
}

void collect_hl_styles(EditorState *e, PointerArray *a, const char *prefix)
{
    const char *dot = strchr(prefix, '.');
    if (!dot || dot == prefix || (dot - prefix) > FILETYPE_NAME_MAX) {
        // No dot found in prefix, or found at offset 0, or buffer too small;
        // just collect matching highlight names added by the `hi` command
        collect_builtin_styles(a, prefix);
        collect_hashmap_keys(&e->styles.other, a, prefix);
        return;
    }

    // Copy and null-terminate the filetype part of `prefix` (before the dot)
    char filetype[FILETYPE_NAME_MAX + 1];
    size_t ftlen = dot - prefix;
    memcpy(filetype, prefix, ftlen);
    filetype[ftlen] = '\0';

    // Find or load the Syntax for `filetype`
    const Syntax *syn = find_syntax(&e->syntaxes, filetype);
    if (!syn) {
        syn = load_syntax_by_filetype(e, filetype);
        if (!syn) {
            return;
        }
    }

    // Collect all emit names from `syn` that start with the string after
    // the dot
    collect_syntax_emit_names(syn, a, dot + 1);
}

void collect_compilers(EditorState *e, PointerArray *a, const char *prefix)
{
    collect_hashmap_keys(&e->compilers, a, prefix);
}

void collect_env (
    char **env, // Pointer to environ(3), or any array with the same format
    PointerArray *a,
    StringView prefix, // Prefix to match against
    const char *suffix // Suffix to append to collected strings
) {
    if (strview_memchr(&prefix, '=')) {
        return;
    }

    size_t sfxlen = strlen(suffix) + 1;
    for (size_t i = 0; env[i]; i++) {
        StringView var = strview(env[i]);
        if (var.length && strview_has_sv_prefix(var, prefix)) {
            size_t pos = 0;
            StringView name = get_delim(var.data, &pos, var.length, '=');
            if (likely(name.length)) {
                ptr_array_append(a, xmemjoin(name.data, name.length, suffix, sfxlen));
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

// Note: `-T` arguments are generated by collect_command_flag_args()
// and completed by collect_completions()
static void complete_bind(EditorState *e, const CommandArgs *a)
{
    // Mask of flags that determine modes (excludes -q)
    CommandFlagSet modemask = cmdargs_flagset_from_str("cnsT");

    if (u64_popcount(a->flag_set & modemask) > 1 || a->nr_flag_args > 1) {
        // Don't complete bindings for multiple modes
        return;
    }

    const ModeHandler *mode;
    if (cmdargs_has_flag(a, 'c')) {
        mode = e->command_mode;
    } else if (cmdargs_has_flag(a, 's')) {
        mode = e->search_mode;
    } else if (cmdargs_has_flag(a, 'T')) {
        BUG_ON(a->nr_flag_args != 1);
        BUG_ON(a->flags[0] != 'T');
        mode = get_mode_handler(&e->modes, a->args[0]);
        if (!mode) {
            return;
        }
    } else {
        mode = e->normal_mode;
    }

    const IntMap *key_bindings = &mode->key_bindings;
    CompletionState *cs = &e->cmdline.completion;
    if (a->nr_args == 0) {
        collect_bound_keys(key_bindings, &cs->completions, cs->parsed);
        return;
    }

    if (a->nr_args != 1 || cs->parsed[0] != '\0') {
        return;
    }

    KeyCode key = keycode_from_str(a->args[a->nr_flag_args]);
    if (key == KEY_NONE) {
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

// Note: `[-ioe]` arguments are generated by collect_command_flag_args()
// and completed by collect_completions()
static void complete_exec(EditorState *e, const CommandArgs *a)
{
    CompletionState *cs = &e->cmdline.completion;
    size_t n = a->nr_args;
    collect_files(e, cs, n == 0 ? COLLECT_EXECUTABLES : COLLECT_ALL);
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

static void complete_def_mode(EditorState *e, const CommandArgs *a)
{
    if (a->nr_args == 0) {
        return;
    }

    CompletionState *cs = &e->cmdline.completion;
    PointerArray *completions = &cs->completions;
    const char *prefix = cs->parsed;
    size_t prefix_len = strlen(prefix);

    for (HashMapIter it = hashmap_iter(&e->modes); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        const ModeHandler *mode = it.entry->value;
        if (!str_has_strn_prefix(name, prefix, prefix_len)) {
            continue;
        }
        if (mode->cmds != &normal_commands) {
            // Exclude command/search mode
            continue;
        }
        if (string_array_contains_str(a->args + 1 + a->nr_flag_args, name)) {
            // Exclude modes already specified in a previous argument
            continue;
        }
        ptr_array_append(completions, xstrdup(name));
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
        collect_hl_styles(e, &cs->completions, cs->parsed);
    } else {
        // TODO: Take into account previous arguments and don't
        // suggest repeat attributes or excess colors
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

static void complete_mode(EditorState *e, const CommandArgs *a)
{
    if (a->nr_args != 0) {
        return;
    }

    CompletionState *cs = &e->cmdline.completion;
    collect_hashmap_keys(&e->modes, &cs->completions, cs->parsed);
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
        collect_env(environ, &cs->completions, strview_from_cstring(cs->parsed), "");
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
    if (!cmdargs_has_flag(a, 'r')) {
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
    {"def-mode", complete_def_mode},
    {"errorfmt", complete_errorfmt},
    {"exec", complete_exec},
    {"ft", complete_ft},
    {"hi", complete_hi},
    {"include", complete_include},
    {"macro", complete_macro},
    {"mode", complete_mode},
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
    CHECK_BSEARCH_ARRAY(completion_handlers, cmd_name);
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
    bool flags_after_nonflags = !(cmd->cmdopts & CMDOPT_NO_FLAGS_AFTER_ARGS);
    if (!can_collect_flags(args, argc, a->nr_flag_args, flags_after_nonflags)) {
        return false;
    }

    const char *flags = cmd->flags;
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

static void collect_command_flag_args (
    EditorState *e,
    PointerArray *array,
    const char *prefix,
    const char *cmd,
    const CommandArgs *a
) {
    char flag = a->flags[0];
    if (streq(cmd, "bind")) {
        WARN_ON(flag != 'T');
        collect_modes(&e->modes, array, prefix);
    } else if (streq(cmd, "exec")) {
        int fd = (flag == 'i') ? 0 : (flag == 'o' ? 1 : 2);
        WARN_ON(fd == 2 && flag != 'e');
        collect_exec_actions(array, prefix, fd);
    }
    // TODO: Completions for `open -e` and `save -e`
}

// Only recurses for cmdname="repeat" and typically not more than once
// NOLINTNEXTLINE(misc-no-recursion)
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

    const char *cmdname = args[0];
    const Command *cmd = find_normal_command(cmdname);
    if (!cmd) {
        return;
    }

    char **args_copy = copy_string_array(args + 1, argc - 1);
    CommandArgs a = cmdargs_new(args_copy);
    ArgParseError err = do_parse_args(cmd, &a);

    if (err == ARGERR_OPTION_ARGUMENT_MISSING) {
        collect_command_flag_args(e, arr, prefix, cmdname, &a);
        goto out;
    }

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

    const CompletionHandler *h = BSEARCH(cmdname, completion_handlers, vstrcmp);
    if (h) {
        h->complete(e, &a);
    } else if (streq(cmdname, "repeat")) {
        if (a.nr_args == 1) {
            collect_normal_commands(arr, prefix);
        } else if (a.nr_args >= 2) {
            collect_completions(e, args + 2, argc - 2);
        }
    }

out:
    free_string_array(args_copy);
}

static bool is_valid_nonbracketed_var_name(StringView name)
{
    AsciiCharType mask = ASCII_ALNUM | ASCII_UNDERSCORE;
    size_t len = name.length;
    return len == 0 || (
        is_alpha_or_underscore(name.data[0])
        && ascii_type_prefix_length(name.data, len, mask) == len
    );
}

static int strptrcmp(const void *v1, const void *v2)
{
    const char *const *s1 = v1;
    const char *const *s2 = v2;
    return strcmp(*s1, *s2);
}

static size_t collect_vars(PointerArray *a, StringView name)
{
    const char *suffix = "";
    size_t pos = STRLEN("$");
    strview_remove_prefix(&name, pos);

    if (strview_remove_matching_prefix(&name, "{")) {
        if (strview_memchr(&name, '}')) {
            return 0;
        }
        collect_builtin_config_variables(a, name);
        pos += STRLEN("{");
        suffix = "}";
    } else if (!is_valid_nonbracketed_var_name(name)) {
        return 0;
    }

    collect_normal_vars(a, name, suffix);
    collect_env(environ, a, name, suffix);
    return pos;
}

static void init_completion(EditorState *e, const CommandLine *cmdline)
{
    CompletionState *cs = &e->cmdline.completion;
    const CommandRunner runner = normal_mode_cmdrunner(e);
    BUG_ON(cs->orig);
    BUG_ON(runner.e != e);
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
            const char *value = runner.lookup_alias(runner.e, name);
            if (value) {
                size_t save = array.count;
                if (parse_commands(&runner, &array, value) != CMDERR_NONE) {
                    for (size_t i = save, n = array.count; i < n; i++) {
                        free(array.ptrs[i]);
                        array.ptrs[i] = NULL;
                    }
                    array.count = save;
                    ptr_array_append(&array, parse_command_arg(&runner, name, end - pos));
                } else {
                    // Remove NULL
                    array.count--;
                }
            } else {
                ptr_array_append(&array, parse_command_arg(&runner, name, end - pos));
            }
            free(name);
        } else {
            ptr_array_append(&array, parse_command_arg(&runner, cmd + pos, end - pos));
        }
        pos = end;
    }

    StringView text = string_view(cmd, cmdline_pos); // Text to be completed
    strview_remove_prefix(&text, completion_pos);

    if (strview_has_prefix(&text, "$")) {
        completion_pos += collect_vars(&cs->completions, text);
    } else {
        cs->escaped = text;
        cs->parsed = parse_command_arg(&runner, text.data, text.length);
        cs->add_space_after_single_match = true;
        size_t count = array.count;
        char **args = count ? (char**)array.ptrs + 1 + semicolon : NULL;
        size_t argc = count ? count - semicolon - 1 : 0;
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
        cs->idx = wrapping_increment(cs->idx, count);
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
        cs->idx = wrapping_decrement(cs->idx, count);
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
    size_t prefix_len = strlen(prefix);
    for (HashMapIter it = hashmap_iter(map); hashmap_next(&it); ) {
        const char *name = it.entry->key;
        if (str_has_strn_prefix(name, prefix, prefix_len)) {
            ptr_array_append(a, xstrdup(name));
        }
    }
}
