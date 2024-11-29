#include "feature.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "tag.h"
#include "error.h"
#include "util/debug.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/time-util.h"
#include "util/xmalloc.h"
#include "util/xreadwrite.h"
#include "util/xstring.h"

static bool tag_is_local_to_file(const Tag *tag, const char *path)
{
    return tag->local && !!path && strview_equal_cstring(&tag->filename, path);
}

static int visibility_cmp(const Tag *a, const Tag *b, const char *filename)
{
    if (!a->local && !b->local) {
        return 0;
    }

    // Is tag visibility limited to the current file?
    bool a_this_file = tag_is_local_to_file(a, filename);
    bool b_this_file = tag_is_local_to_file(b, filename);

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
        // a is more interesting because it's a local symbol
        return -1;
    }
    if (b->local && b_this_file) {
        // b is more interesting because it's a local symbol
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

static int tag_cmp_r(const void *ap, const void *bp, void *userdata)
{
    const Tag *const *a = ap;
    const Tag *const *b = bp;
    const char *filename = userdata;
    int r = visibility_cmp(*a, *b, filename);
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
            error_msg("failed to open '%s': %s", path, strerror(errno));
            return -1;
        }
        *slash = '\0';
    }

    error_msg("no tags file");
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
    char path[8192];
    if (unlikely(!getcwd(path, sizeof(path) - STRLEN("/tags")))) {
        return error_msg_errno("getcwd");
    }

    int fd = open_tag_file(path);
    if (fd < 0) {
        return false;
    }

    struct stat st;
    if (unlikely(fstat(fd, &st) != 0)) {
        const char *str = strerror(errno);
        xclose(fd);
        return error_msg("fstat: %s", str);
    }

    if (unlikely(st.st_size <= 0)) {
        xclose(fd);
        return error_msg("empty tags file");
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
        xclose(fd);
        return error_msg("malloc: %s", strerror(ENOMEM));
    }

    ssize_t size = xread_all(fd, buf, st.st_size);
    int err = errno;
    xclose(fd);
    if (size < 0) {
        free(buf);
        return error_msg("read: %s", strerror(err));
    }

    *tf = (TagFile) {
        .filename = xstrdup(path),
        .dirname_len = (xstrrchr(path, '/') - path) + 1, // Includes last slash
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

#if !HAVE_QSORT_R
static const char *current_filename_global; // NOLINT(*-non-const-global-variables)
static int tag_cmp(const void *ap, const void *bp)
{
    return tag_cmp_r(ap, bp, (char*)current_filename_global);
}
#endif

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

    if (tags->count < 2) {
        return;
    }

    if (filename) {
        BUG_ON(!path_is_absolute(filename));
        size_t n = tf->dirname_len;
        BUG_ON(n == 0);
        if (strncmp(filename, tf->filename, n) == 0) {
            filename += n;
        } else {
            // Filename doesn't start with directory prefix of tag file
            filename = NULL;
        }
    }

    void **ptrs = tags->ptrs;
    BUG_ON(!ptrs);

#if HAVE_QSORT_R
    qsort_r(ptrs, tags->count, sizeof(*ptrs), tag_cmp_r, (char*)filename);
#else
    current_filename_global = filename;
    qsort(ptrs, tags->count, sizeof(*ptrs), tag_cmp);
    current_filename_global = NULL;
#endif
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

    char *filename = path_join_sv(dir, &tag->filename, false);
    m->loc = new_file_location(filename, 0, 0, 0);

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

    // Note that `dirname_len` always includes a trailing slash, but the
    // call to path_join_sv() in add_message_for_tag() handles that fine
    BUG_ON(tf->dirname_len == 0);
    StringView tagfile_dir = string_view(tf->filename, tf->dirname_len);

    for (size_t i = 0; i < ntags; i++) {
        Tag *tag = tags.ptrs[i];
        add_message_for_tag(messages, tag, &tagfile_dir);
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

String dump_tags(TagFile *tf)
{
    if (!load_tag_file(tf)) {
        return string_new(0);
    }

    const struct timespec ts = {.tv_sec = tf->mtime};
    char sizestr[FMT_FILESIZE_MAX];
    char tstr[TIME_STR_BUFSIZE];
    String buf = string_new(tf->size);

    string_sprintf (
        &buf,
        "Tags file\n---------\n\n"
        "%s %s\n%s %s\n%s %s\n\n"
        "Tag entries\n-----------\n\n",
        "     Path:", tf->filename,
        " Modified:", timespec_to_str(&ts, tstr, sizeof(tstr)) ? tstr : "-",
        "     Size:", filesize_to_str(tf->size, sizestr)
    );

    const StringView prefix = STRING_VIEW_INIT;
    size_t pos = 0;
    Tag tag;

    while (next_tag(tf->buf, tf->size, &pos, &prefix, false, &tag)) {
        string_append_buf(&buf, tag.name.data, tag.name.length);
        string_append_cstring(&buf, "   ");
        string_append_buf(&buf, tag.filename.data, tag.filename.length);
        if (tag.kind) {
            string_sprintf(&buf, "   kind:%c", tag.kind);
        }
        if (tag.local) {
            string_append_cstring(&buf, "   LOCAL");
        }
        if (tag.pattern) {
            string_sprintf(&buf, "   /%s/", tag.pattern);
        } else {
            string_sprintf(&buf, "   lineno:%lu", tag.lineno);
        }
        string_append_byte(&buf, '\n');
        free_tag(&tag);
    }

    return buf;
}
