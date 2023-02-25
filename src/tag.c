#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tag.h"
#include "error.h"
#include "util/debug.h"
#include "util/log.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"

static const char *current_filename; // For sorting tags

static int visibility_cmp(const Tag *a, const Tag *b)
{
    bool a_this_file = false;
    bool b_this_file = false;

    if (!a->local && !b->local) {
        return 0;
    }

    // Is tag visibility limited to the current file?
    if (a->local) {
        a_this_file = current_filename && strview_equal_cstring(&a->filename, current_filename);
    }
    if (b->local) {
        b_this_file = current_filename && strview_equal_cstring(&b->filename, current_filename);
    }

    // Tags local to other file than current are not interesting
    if (a->local && !a_this_file) {
        // a is not interesting
        if (b->local && !b_this_file) {
            // b is equally uninteresting
            return 0;
        }
        // b is more interesting, sort it before a
        return 1;
    }
    if (b->local && !b_this_file) {
        // b is not interesting
        return -1;
    }

    // Both are NOT UNinteresting

    if (a->local && a_this_file) {
        if (b->local && b_this_file) {
            return 0;
        }
        // a is more interesting because it is local symbol
        return -1;
    }
    if (b->local && b_this_file) {
        // b is more interesting because it is local symbol
        return 1;
    }
    return 0;
}

static int kind_cmp(const Tag *a, const Tag *b)
{
    if (a->kind == b->kind) {
        return 0;
    }

    // Struct member (m) is not very interesting
    if (a->kind == 'm') {
        return 1;
    }
    if (b->kind == 'm') {
        return -1;
    }

    // Global variable (v) is not very interesting
    if (a->kind == 'v') {
        return 1;
    }
    if (b->kind == 'v') {
        return -1;
    }

    // Struct (s), union (u)
    return 0;
}

static int tag_cmp(const void *ap, const void *bp)
{
    const Tag *const *a = ap;
    const Tag *const *b = bp;
    int r = visibility_cmp(*a, *b);
    return r ? r : kind_cmp(*a, *b);
}

// Find "tags" file from directory path and its parent directories
static int open_tag_file(char *path)
{
    static const char tags[] = "tags";
    while (*path) {
        size_t len = strlen(path);
        char *slash = strrchr(path, '/');
        if (slash != path + len - 1) {
            path[len++] = '/';
        }
        memcpy(path + len, tags, sizeof(tags));
        int fd = xopen(path, O_RDONLY | O_CLOEXEC, 0);
        if (fd >= 0) {
            return fd;
        }
        if (errno != ENOENT) {
            return -1;
        }
        *slash = '\0';
    }
    errno = ENOENT;
    return -1;
}

static bool tag_file_changed (
    const TagFile *tf,
    const char *filename,
    const struct stat *st
) {
    return tf->mtime != st->st_mtime || !streq(tf->filename, filename);
}

// Note: does not free `tf` itself
void tag_file_free(TagFile *tf)
{
    free(tf->filename);
    free(tf->buf);
    *tf = (TagFile){.filename = NULL};
}

static bool load_tag_file(TagFile *tf)
{
    char path[4096];
    if (unlikely(!getcwd(path, sizeof(path) - STRLEN("/tags")))) {
        LOG_ERRNO("getcwd");
        return false;
    }

    int fd = open_tag_file(path);
    if (fd < 0) {
        return false;
    }

    struct stat st;
    if (unlikely(fstat(fd, &st) != 0)) {
        LOG_ERRNO("fstat");
        xclose(fd);
        return false;
    }

    if (unlikely(st.st_size <= 0)) {
        xclose(fd);
        return false;
    }

    if (tf->filename) {
        if (!tag_file_changed(tf, path, &st)) {
            xclose(fd);
            return true;
        }
        tag_file_free(tf);
        BUG_ON(tf->filename);
    }

    char *buf = malloc(st.st_size);
    if (unlikely(!buf)) {
        LOG_ERRNO("malloc");
        xclose(fd);
        return false;
    }

    ssize_t size = xread_all(fd, buf, st.st_size);
    xclose(fd);
    if (size < 0) {
        free(buf);
        return false;
    }

    *tf = (TagFile) {
        .filename = xstrdup(path),
        .buf = buf,
        .size = size,
        .mtime = st.st_mtime,
    };

    return true;
}

static void free_tags_cb(Tag *t)
{
    free_tag(t);
    free(t);
}

static void free_tags(PointerArray *tags)
{
    ptr_array_free_cb(tags, FREE_FUNC(free_tags_cb));
}

// Both parameters must be absolute and clean
static const char *path_slice_relative(const char *filename, const StringView dir)
{
    if (strncmp(filename, dir.data, dir.length) != 0) {
        // Filename doesn't start with dir
        return NULL;
    }
    switch (filename[dir.length]) {
    case '\0': // Equal strings
        return ".";
    case '/':
        return filename + dir.length + 1;
    }
    return NULL;
}

static void tag_file_find_tags (
    const TagFile *tf,
    const char *filename,
    const StringView *name,
    PointerArray *tags
) {
    Tag *t = xnew(Tag, 1);
    size_t pos = 0;
    while (next_tag(tf->buf, tf->size, &pos, name, true, t)) {
        ptr_array_append(tags, t);
        t = xnew(Tag, 1);
    }
    free(t);

    if (!filename) {
        current_filename = NULL;
    } else {
        StringView dir = path_slice_dirname(tf->filename);
        current_filename = path_slice_relative(filename, dir);
    }
    ptr_array_sort(tags, tag_cmp);
    current_filename = NULL;
}

// Note: this moves ownership of tag->pattern to the generated Message
// and assigns NULL to the old pointer
void add_message_for_tag(MessageArray *messages, Tag *tag, const StringView *dir)
{
    BUG_ON(dir->length == 0);
    BUG_ON(dir->data[0] != '/');

    static const char prefix[] = "Tag ";
    size_t prefix_len = sizeof(prefix) - 1;
    size_t msg_len = prefix_len + tag->name.length;
    Message *m = xmalloc(sizeof(*m) + msg_len + 1);

    memcpy(m->msg, prefix, prefix_len);
    memcpy(m->msg + prefix_len, tag->name.data, tag->name.length);
    m->msg[msg_len] = '\0';

    m->loc = xnew0(FileLocation, 1);
    m->loc->filename = path_join_sv(dir, &tag->filename, false);

    if (tag->pattern) {
        m->loc->pattern = tag->pattern; // Message takes ownership
        tag->pattern = NULL;
    } else {
        m->loc->line = tag->lineno;
    }

    add_message(messages, m);
}

size_t tag_lookup(TagFile *tf, const StringView *name, const char *filename, MessageArray *messages)
{
    clear_messages(messages);
    if (!load_tag_file(tf)) {
        error_msg("No tags file");
        return 0;
    }

    // Filename helps to find correct tags
    PointerArray tags = PTR_ARRAY_INIT;
    tag_file_find_tags(tf, filename, name, &tags);

    size_t ntags = tags.count;
    if (ntags == 0) {
        error_msg("Tag '%.*s' not found", (int)name->length, name->data);
        return 0;
    }

    StringView tf_dir = path_slice_dirname(tf->filename);
    for (size_t i = 0; i < ntags; i++) {
        Tag *tag = tags.ptrs[i];
        add_message_for_tag(messages, tag, &tf_dir);
    }

    free_tags(&tags);
    return ntags;
}

void collect_tags(TagFile *tf, PointerArray *a, const StringView *prefix)
{
    if (!load_tag_file(tf)) {
        return;
    }

    Tag t;
    size_t pos = 0;
    StringView prev = STRING_VIEW_INIT;
    while (next_tag(tf->buf, tf->size, &pos, prefix, false, &t)) {
        BUG_ON(t.name.length == 0);
        if (prev.length == 0 || !strview_equal(&t.name, &prev)) {
            ptr_array_append(a, xstrcut(t.name.data, t.name.length));
            prev = t.name;
        }
        free_tag(&t);
    }
}
