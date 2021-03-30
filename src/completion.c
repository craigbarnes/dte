#include <dirent.h>
#include <sys/stat.h>
#include "completion.h"
#include "alias.h"
#include "bind.h"
#include "cmdline.h"
#include "command/args.h"
#include "command/env.h"
#include "command/parse.h"
#include "command/serialize.h"
#include "commands.h"
#include "compiler.h"
#include "config.h"
#include "editor.h"
#include "filetype.h"
#include "options.h"
#include "show.h"
#include "syntax/color.h"
#include "tag.h"
#include "util/ascii.h"
#include "util/debug.h"
#include "util/path.h"
#include "util/ptr-array.h"
#include "util/str-util.h"
#include "util/string.h"
#include "util/xmalloc.h"

static struct {
    // Part of string that is to be replaced
    char *escaped;
    char *parsed;

    char *head;
    char *tail;
    PointerArray completions;
    size_t idx;

    // Should we add space after completed string if we have only one match?
    bool add_space;

    bool tilde_expanded;
} completion;

static int strptrcmp(const void *v1, const void *v2)
{
    const char *const *s1 = v1;
    const char *const *s2 = v2;
    return strcmp(*s1, *s2);
}

static void sort_completions(void)
{
    ptr_array_sort(&completion.completions, strptrcmp);
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

static void collect_files(bool directories_only)
{
    char *str = parse_command_arg (
        completion.escaped,
        strlen(completion.escaped),
        false
    );

    if (!streq(completion.parsed, str)) {
        // ~ was expanded
        const char *fileprefix = path_basename(str);

        completion.tilde_expanded = true;
        if (fileprefix == str) {
            // str doesn't contain slashes
            // complete ~ to ~/ or ~user to ~user/
            size_t len = strlen(str);
            char *s = xmalloc(len + 2);
            memcpy(s, str, len);
            s[len] = '/';
            s[len + 1] = '\0';
            add_completion(s);
        } else {
            char *dir = path_dirname(completion.parsed);
            char *dirprefix = path_dirname(str);
            do_collect_files(dir, dirprefix, fileprefix, directories_only);
            free(dirprefix);
            free(dir);
        }
    } else {
        const char *fileprefix = path_basename(completion.parsed);

        if (fileprefix == completion.parsed) {
            // completion.parsed doesn't contain slashes
            do_collect_files(".", "", fileprefix, directories_only);
        } else {
            char *dir = path_dirname(completion.parsed);
            do_collect_files(dir, dir, fileprefix, directories_only);
            free(dir);
        }
    }
    free(str);

    if (completion.completions.count == 1) {
        // Add space if completed string is not a directory
        const char *s = completion.completions.ptrs[0];
        size_t len = strlen(s);
        if (len > 0) {
            completion.add_space = s[len - 1] != '/';
        }
    }
}

static void collect_str(const char *const strs[], size_t n, const char *prefix)
{
    for (size_t i = 0; i < n; i++) {
        if (str_has_prefix(strs[i], prefix)) {
            add_completion(xstrdup(strs[i]));
        }
    }
}

static void collect_completions(char **args, size_t argc)
{
    if (!argc) {
        collect_normal_commands(completion.parsed);
        collect_aliases(completion.parsed);
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

    const StringView cmd_name = strview_from_cstring(args[0]);

    if (
        strview_equal_cstring(&cmd_name, "save")
        || strview_equal_cstring(&cmd_name, "run")
        || strview_equal_cstring(&cmd_name, "pipe-from")
        || strview_equal_cstring(&cmd_name, "pipe-to")
    ) {
        collect_files(false);
    } else if (strview_equal_cstring(&cmd_name, "open")) {
        if (!cmdargs_has_flag(&a, 't')) {
            collect_files(false);
        }
    } else if (strview_equal_cstring(&cmd_name, "wsplit")) {
        if (!cmdargs_has_flag(&a, 't') && !cmdargs_has_flag(&a, 'n')) {
            collect_files(false);
        }
    } else if (strview_equal_cstring(&cmd_name, "cd")) {
        collect_files(true);
    } else if (strview_equal_cstring(&cmd_name, "include")) {
        if (a.nr_args == 0) {
            if (cmdargs_has_flag(&a, 'b')) {
                collect_builtin_configs(completion.parsed);
            } else {
                collect_files(false);
            }
        }
    } else if (strview_equal_cstring(&cmd_name, "ft")) {
        if (a.nr_args == 0) {
            collect_ft(completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "hi")) {
        if (a.nr_args == 0) {
            collect_hl_colors(completion.parsed);
        } else {
            collect_colors_and_attributes(completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "option")) {
        if (a.nr_args == 0) {
            if (!cmdargs_has_flag(&a, 'r')) {
                collect_ft(completion.parsed);
            }
        } else if (a.nr_args & 1) {
            collect_auto_options(completion.parsed);
        } else {
            collect_option_values(a.args[a.nr_args - 1], completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "set")) {
        if ((a.nr_args + 1) & 1) {
            bool local = cmdargs_has_flag(&a, 'l');
            bool global = cmdargs_has_flag(&a, 'g');
            collect_options(completion.parsed, local, global);
        } else {
            collect_option_values(a.args[a.nr_args - 1], completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "setenv")) {
        if (a.nr_args == 0) {
            collect_env(completion.parsed);
        } else if (a.nr_args == 1 && completion.parsed[0] == '\0') {
            BUG_ON(!a.args[0]);
            const char *value = getenv(a.args[0]);
            if (value) {
                add_completion(xstrdup(value));
            }
        }
    } else if (strview_equal_cstring(&cmd_name, "toggle")) {
        if (a.nr_args == 0) {
            bool global = cmdargs_has_flag(&a, 'g');
            collect_toggleable_options(completion.parsed, global);
        }
    } else if (strview_equal_cstring(&cmd_name, "tag")) {
        if (a.nr_args == 0 && !cmdargs_has_flag(&a, 'r')) {
            TagFile *tf = load_tag_file();
            if (tf) {
                collect_tags(tf, completion.parsed);
            }
        }
    } else if (strview_equal_cstring(&cmd_name, "compile")) {
        if (a.nr_args == 0) {
            collect_compilers(completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "errorfmt")) {
        static const char *const names[] = {
            "file", "line", "column", "message", "_"
        };
        if (a.nr_args == 0) {
            collect_compilers(completion.parsed);
        } else if (a.nr_args >= 2 && !cmdargs_has_flag(&a, 'i')) {
            collect_str(names, ARRAY_COUNT(names), completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "repeat")) {
        if (a.nr_args == 1) {
            collect_normal_commands(completion.parsed);
        } else if (a.nr_args >= 2) {
            collect_completions(args + 2, argc - 2);
        }
    } else if (strview_equal_cstring(&cmd_name, "show")) {
        if (a.nr_args == 0) {
            collect_show_subcommands(completion.parsed);
        } else if (a.nr_args == 1) {
            BUG_ON(!a.args[0]);
            collect_show_subcommand_args(a.args[0], completion.parsed);
        }
    } else if (strview_equal_cstring(&cmd_name, "macro")) {
        static const char *const verbs[] = {
            "record", "stop", "toggle", "cancel", "play"
        };
        if (a.nr_args == 0) {
            collect_str(verbs, ARRAY_COUNT(verbs), completion.parsed);
        }
    }

out:
    free_string_array(args_copy);
}

static void init_completion(void)
{
    const char *cmd = string_borrow_cstring(&editor.cmdline.buf);
    PointerArray array = PTR_ARRAY_INIT;
    ssize_t semicolon = -1;
    ssize_t completion_pos = -1;
    size_t pos = 0;

    while (1) {
        while (ascii_isspace(cmd[pos])) {
            pos++;
        }

        if (pos >= editor.cmdline.pos) {
            completion_pos = editor.cmdline.pos;
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
        if (err != CMDERR_NONE || end >= editor.cmdline.pos) {
            completion_pos = pos;
            break;
        }

        if (semicolon + 1 == array.count) {
            char *name = xstrslice(cmd, pos, end);
            const char *value = find_alias(name);

            if (value) {
                size_t save = array.count;
                if (parse_commands(&array, value) != CMDERR_NONE) {
                    for (size_t i = save, n = array.count; i < n; i++) {
                        free(array.ptrs[i]);
                        array.ptrs[i] = NULL;
                    }
                    array.count = save;
                    ptr_array_append(&array, parse_command_arg(name, end - pos, true));
                } else {
                    // Remove NULL
                    array.count--;
                }
            } else {
                ptr_array_append(&array, parse_command_arg(name, end - pos, true));
            }
            free(name);
        } else {
            ptr_array_append(&array, parse_command_arg(cmd + pos, end - pos, true));
        }
        pos = end;
    }

    const char *str = cmd + completion_pos;
    size_t len = editor.cmdline.pos - completion_pos;
    if (len && str[0] == '$') {
        bool var = true;
        if (len >= 2) {
            if (is_alpha_or_underscore(str[1])) {
                for (size_t i = 2; i < len; i++) {
                    if (is_alnum_or_underscore(str[i])) {
                        continue;
                    }
                    var = false;
                    break;
                }
            } else {
                var = false;
            }
        }
        if (var) {
            char *name = xstrslice(str, 1, len);
            completion_pos++;
            completion.escaped = NULL;
            completion.parsed = NULL;
            completion.head = xstrcut(cmd, completion_pos);
            completion.tail = xstrdup(cmd + editor.cmdline.pos);
            collect_env(name);
            collect_builtin_env(name);
            sort_completions();
            free(name);
            ptr_array_free(&array);
            return;
        }
    }

    completion.escaped = xstrcut(str, len);
    completion.parsed = parse_command_arg(completion.escaped, len, true);
    completion.head = xstrcut(cmd, completion_pos);
    completion.tail = xstrdup(cmd + editor.cmdline.pos);
    completion.add_space = true;

    char **args = NULL;
    size_t argc = 0;
    if (array.count) {
        args = (char**)array.ptrs + 1 + semicolon;
        argc = array.count - semicolon - 1;
    }

    collect_completions(args, argc);
    sort_completions();
    ptr_array_free(&array);
}

static void do_complete_command(void)
{
    const char *arg = completion.completions.ptrs[completion.idx];
    char *middle = escape_command_arg(arg, !completion.tilde_expanded);
    size_t middle_len = strlen(middle);
    size_t head_len = strlen(completion.head);
    size_t tail_len = strlen(completion.tail);

    char *str = xmalloc(head_len + middle_len + tail_len + 2);
    memcpy(str, completion.head, head_len);
    memcpy(str + head_len, middle, middle_len);
    if (completion.completions.count == 1 && completion.add_space) {
        str[head_len + middle_len] = ' ';
        middle_len++;
    }
    memcpy(str + head_len + middle_len, completion.tail, tail_len + 1);

    cmdline_set_text(&editor.cmdline, str);
    editor.cmdline.pos = head_len + middle_len;

    free(middle);
    free(str);
    if (completion.completions.count == 1) {
        reset_completion();
    }
}

void complete_command_next(void)
{
    const bool init = !completion.head;
    if (init) {
        init_completion();
    }
    if (!completion.completions.count) {
        return;
    }
    if (!init) {
        if (completion.idx >= completion.completions.count - 1) {
            completion.idx = 0;
        } else {
            completion.idx++;
        }
    }
    do_complete_command();
}

void complete_command_prev(void)
{
    const bool init = !completion.head;
    if (init) {
        init_completion();
    }
    if (!completion.completions.count) {
        return;
    }
    if (!init) {
        if (completion.idx == 0) {
            completion.idx = completion.completions.count - 1;
        } else {
            completion.idx--;
        }
    }
    do_complete_command();
}

void reset_completion(void)
{
    free(completion.escaped);
    free(completion.parsed);
    free(completion.head);
    free(completion.tail);
    ptr_array_free(&completion.completions);
    MEMZERO(&completion);
}
