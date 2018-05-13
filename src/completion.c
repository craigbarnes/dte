#include "completion.h"
#include "command.h"
#include "cmdline.h"
#include "editor.h"
#include "options.h"
#include "alias.h"
#include "str.h"
#include "ptr-array.h"
#include "tag.h"
#include "common.h"
#include "color.h"
#include "env.h"
#include "path.h"
#include "config.h"

static struct {
    // Part of string which is to be replaced
    char *escaped;
    char *parsed;

    char *head;
    char *tail;
    PointerArray completions;
    int idx;

    // Should we add space after completed string if we have only one match?
    bool add_space;

    bool tilde_expanded;
} completion;

static int strptrcmp(const void *ap, const void *bp)
{
    const char *a = *(const char **)ap;
    const char *b = *(const char **)bp;
    return strcmp(a, b);
}

static void sort_completions(void)
{
    PointerArray *a = &completion.completions;
    if (a->count > 1) {
        BUG_ON(!a->ptrs);
        qsort(a->ptrs, a->count, sizeof(*a->ptrs), strptrcmp);
    }
}

void add_completion(char *str)
{
    ptr_array_add(&completion.completions, str);
}

static void collect_commands(const char *prefix)
{
    for (size_t i = 0; commands[i].name; i++) {
        const Command *c = &commands[i];
        if (str_has_prefix(c->name, prefix)) {
            add_completion(xstrdup(c->name));
        }
    }
    collect_aliases(prefix);
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
            if (strncmp(name, fileprefix, flen)) {
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

        String buf = STRING_INIT;
        if (dirprefix[0]) {
            string_add_str(&buf, dirprefix);
            if (!str_has_suffix(dirprefix, "/")) {
                string_add_byte(&buf, '/');
            }
        }
        string_add_str(&buf, name);
        if (is_dir) {
            string_add_byte(&buf, '/');
        }
        add_completion(string_steal_cstring(&buf));
    }
    closedir(dir);
}

static void collect_files(bool directories_only)
{
    char *str = parse_command_arg(completion.escaped, false);

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
            s[len + 1] = 0;
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

static void collect_env(const char *prefix)
{
    extern char **environ;

    for (size_t i = 0; environ[i]; i++) {
        const char *e = environ[i];

        if (str_has_prefix(e, prefix)) {
            const char *end = strchr(e, '=');
            if (end) {
                add_completion(xstrslice(e, 0, end - e));
            }
        }
    }
    collect_builtin_env(prefix);
}

static void collect_completions(char **args, int argc)
{
    if (!argc) {
        collect_commands(completion.parsed);
        return;
    }

    const Command *cmd = find_command(commands, args[0]);
    if (!cmd) {
        return;
    }

    if (
        streq(cmd->name, "open")
        || streq(cmd->name, "wsplit")
        || streq(cmd->name, "save")
        || streq(cmd->name, "compile")
        || streq(cmd->name, "run")
        || streq(cmd->name, "pass-through")
        || streq(cmd->name, "lua-file")
    ) {
        collect_files(false);
        return;
    }
    if (streq(cmd->name, "cd")) {
        collect_files(true);
        return;
    }
    if (streq(cmd->name, "include")) {
        switch (argc) {
        case 1:
            collect_files(false);
            break;
        case 2:
            if (streq(args[1], "-b")) {
                collect_builtin_configs(completion.parsed, false);
            }
            break;
        }
        return;
    }
    if (streq(cmd->name, "insert-builtin")) {
        if (argc == 1) {
            collect_builtin_configs(completion.parsed, true);
        }
        return;
    }
    if (streq(cmd->name, "hi")) {
        switch (argc) {
        case 1:
            collect_hl_colors(completion.parsed);
            break;
        default:
            collect_colors_and_attributes(completion.parsed);
            break;
        }
        return;
    }
    if (streq(cmd->name, "set")) {
        if (argc % 2) {
            collect_options(completion.parsed);
        } else {
            collect_option_values(args[argc - 1], completion.parsed);
        }
        return;
    }
    if (streq(cmd->name, "toggle") && argc == 1) {
        collect_toggleable_options(completion.parsed);
        return;
    }
    if (streq(cmd->name, "tag") && argc == 1) {
        TagFile *tf = load_tag_file();
        if (tf != NULL) {
            collect_tags(tf, completion.parsed);
        }
        return;
    }
}

static void init_completion(void)
{
    char *cmd = string_cstring(&editor.cmdline.buf);
    PointerArray array = PTR_ARRAY_INIT;
    int semicolon = -1;
    int completion_pos = -1;
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
            ptr_array_add(&array, NULL);
            pos++;
            continue;
        }

        Error *err = NULL;
        size_t end = find_end(cmd, pos, &err);
        if (err || end >= editor.cmdline.pos) {
            error_free(err);
            completion_pos = pos;
            break;
        }

        if (semicolon + 1 == array.count) {
            char *name = xstrslice(cmd, pos, end);
            const char *value = find_alias(name);

            if (value) {
                size_t i, save = array.count;

                if (!parse_commands(&array, value, &err)) {
                    error_free(err);
                    for (i = save; i < array.count; i++) {
                        free(array.ptrs[i]);
                        array.ptrs[i] = NULL;
                    }
                    array.count = save;
                    ptr_array_add(&array, parse_command_arg(name, true));
                } else {
                    // Remove NULL
                    array.count--;
                }
            } else {
                ptr_array_add(&array, parse_command_arg(name, true));
            }
            free(name);
        } else {
            ptr_array_add(&array, parse_command_arg(cmd + pos, true));
        }
        pos = end;
    }

    const char *str = cmd + completion_pos;
    size_t len = editor.cmdline.pos - completion_pos;
    if (len && str[0] == '$') {
        bool var = true;
        for (size_t i = 1; i < len; i++) {
            char ch = str[i];
            if (ascii_isalpha(ch) || ch == '_') {
                continue;
            }
            if (i > 1 && ascii_isdigit(ch)) {
                continue;
            }

            var = false;
            break;
        }
        if (var) {
            char *name = xstrslice(str, 1, len);
            completion_pos++;
            completion.escaped = NULL;
            completion.parsed = NULL;
            completion.head = xstrslice(cmd, 0, completion_pos);
            completion.tail = xstrdup(cmd + editor.cmdline.pos);
            collect_env(name);
            sort_completions();
            free(name);
            ptr_array_free(&array);
            free(cmd);
            return;
        }
    }

    completion.escaped = xstrslice(str, 0, len);
    completion.parsed = parse_command_arg(completion.escaped, true);
    completion.head = xstrslice(cmd, 0, completion_pos);
    completion.tail = xstrdup(cmd + editor.cmdline.pos);
    completion.add_space = true;

    collect_completions (
        (char **)array.ptrs + semicolon + 1,
        array.count - semicolon - 1
    );
    sort_completions();
    ptr_array_free(&array);
    free(cmd);
}

static char *escape(const char *str)
{
    String buf = STRING_INIT;

    if (!str[0]) {
        return xstrdup("\"\"");
    }

    if (str[0] == '~' && !completion.tilde_expanded) {
        string_add_ch(&buf, '\\');
    }

    for (size_t i = 0; str[i]; i++) {
        char ch = str[i];
        switch (ch) {
        case ' ':
        case '"':
        case '$':
        case '\'':
        case '*':
        case ';':
        case '?':
        case '[':
        case '\\':
        case '{':
            string_add_ch(&buf, '\\');
            string_add_byte(&buf, ch);
            break;
        default:
            string_add_byte(&buf, ch);
            break;
        }
    }
    return string_steal_cstring(&buf);
}

void complete_command(void)
{
    if (!completion.head) {
        init_completion();
    }
    if (!completion.completions.count) {
        return;
    }

    char *middle = escape(completion.completions.ptrs[completion.idx]);
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
    completion.idx = (completion.idx + 1) % completion.completions.count;
    if (completion.completions.count == 1) {
        reset_completion();
    }
}

void reset_completion(void)
{
    free(completion.escaped);
    free(completion.parsed);
    free(completion.head);
    free(completion.tail);
    ptr_array_free(&completion.completions);
    memzero(&completion);
}
