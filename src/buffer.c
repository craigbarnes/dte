#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "buffer.h"
#include "editor.h"
#include "file-option.h"
#include "filetype.h"
#include "lock.h"
#include "syntax/state.h"
#include "util/debug.h"
#include "util/intern.h"
#include "util/log.h"
#include "util/numtostr.h"
#include "util/path.h"
#include "util/str-util.h"
#include "util/time-util.h"
#include "util/xmalloc.h"

void set_display_filename(Buffer *buffer, char *name)
{
    free(buffer->display_filename);
    buffer->display_filename = name;
}

/*
 * Mark line range min...max (inclusive) "changed". These lines will be
 * redrawn when screen is updated. This is called even when content has not
 * been changed, but selection has or line has been deleted and all lines
 * after the deleted line move up.
 *
 * Syntax highlighter has different logic. It cares about contents of the
 * lines, not about selection or if the lines have been moved up or down.
 */
void buffer_mark_lines_changed(Buffer *buffer, long min, long max)
{
    if (min > max) {
        long tmp = min;
        min = max;
        max = tmp;
    }
    buffer->changed_line_min = MIN(min, buffer->changed_line_min);
    buffer->changed_line_max = MAX(max, buffer->changed_line_max);
}

const char *buffer_filename(const Buffer *buffer)
{
    const char *name = buffer->display_filename;
    return name ? name : "(No name)";
}

void buffer_set_encoding(Buffer *buffer, Encoding encoding, bool utf8_bom)
{
    if (
        buffer->encoding.type != encoding.type
        || buffer->encoding.name != encoding.name
    ) {
        const EncodingType type = encoding.type;
        if (type == UTF8) {
            buffer->bom = utf8_bom;
        } else {
            buffer->bom = type < NR_ENCODING_TYPES && !!get_bom_for_encoding(type);
        }
        buffer->encoding = encoding;
    }
}

Buffer *buffer_new(PointerArray *buffers, const GlobalOptions *gopts, const Encoding *encoding)
{
    static unsigned long id;
    Buffer *buffer = xnew0(Buffer, 1);
    list_init(&buffer->blocks);
    buffer->cur_change = &buffer->change_head;
    buffer->saved_change = &buffer->change_head;
    buffer->id = ++id;
    buffer->crlf_newlines = gopts->crlf_newlines;

    if (encoding) {
        buffer_set_encoding(buffer, *encoding, gopts->utf8_bom);
    } else {
        buffer->encoding.type = ENCODING_AUTODETECT;
    }

    static_assert(sizeof(*gopts) >= sizeof(CommonOptions));
    memcpy(&buffer->options, gopts, sizeof(CommonOptions));
    buffer->options.brace_indent = 0;
    buffer->options.filetype = str_intern("none");
    buffer->options.indent_regex = NULL;

    ptr_array_append(buffers, buffer);
    return buffer;
}

Buffer *open_empty_buffer(PointerArray *buffers, const GlobalOptions *gopts)
{
    Encoding enc = encoding_from_type(UTF8);
    Buffer *buffer = buffer_new(buffers, gopts, &enc);

    // At least one block required
    Block *blk = block_new(1);
    list_add_before(&blk->node, &buffer->blocks);

    return buffer;
}

void free_blocks(Buffer *buffer)
{
    ListHead *item = buffer->blocks.next;
    while (item != &buffer->blocks) {
        ListHead *next = item->next;
        Block *blk = BLOCK(item);
        free(blk->data);
        free(blk);
        item = next;
    }
}

void free_buffer(Buffer *buffer)
{
    if (buffer->locked) {
        unlock_file(buffer->abs_filename);
    }

    free_changes(&buffer->change_head);
    free(buffer->line_start_states.ptrs);
    free(buffer->views.ptrs);
    free(buffer->display_filename);
    free(buffer->abs_filename);

    if (buffer->stdout_buffer) {
        return;
    }

    free_blocks(buffer);
    free(buffer);
}

void remove_and_free_buffer(PointerArray *buffers, Buffer *buffer)
{
    ptr_array_remove(buffers, buffer);
    free_buffer(buffer);
}

static bool same_file(const Buffer *buffer, const struct stat *st)
{
    return (st->st_dev == buffer->file.dev) && (st->st_ino == buffer->file.ino);
}

Buffer *find_buffer(const PointerArray *buffers, const char *abs_filename)
{
    struct stat st;
    bool st_ok = stat(abs_filename, &st) == 0;
    for (size_t i = 0, n = buffers->count; i < n; i++) {
        Buffer *buffer = buffers->ptrs[i];
        const char *f = buffer->abs_filename;
        if ((f && streq(f, abs_filename)) || (st_ok && same_file(buffer, &st))) {
            return buffer;
        }
    }
    return NULL;
}

Buffer *find_buffer_by_id(const PointerArray *buffers, unsigned long id)
{
    for (size_t i = 0, n = buffers->count; i < n; i++) {
        Buffer *buffer = buffers->ptrs[i];
        if (buffer->id == id) {
            return buffer;
        }
    }
    return NULL;
}

bool buffer_detect_filetype(Buffer *buffer, const PointerArray *filetypes)
{
    StringView line = STRING_VIEW_INIT;
    if (BLOCK(buffer->blocks.next)->size) {
        BlockIter bi = block_iter(buffer);
        fill_line_ref(&bi, &line);
    } else if (!buffer->abs_filename) {
        return false;
    }

    const char *ft = find_ft(filetypes, buffer->abs_filename, line);
    if (ft && !streq(ft, buffer->options.filetype)) {
        buffer->options.filetype = str_intern(ft);
        return true;
    }

    return false;
}

void update_short_filename_cwd(Buffer *buffer, const StringView *home, const char *cwd)
{
    const char *abs = buffer->abs_filename;
    if (!abs) {
        return;
    }
    char *name = cwd ? short_filename_cwd(abs, cwd, home) : xstrdup(abs);
    set_display_filename(buffer, name);
}

void update_short_filename(Buffer *buffer, const StringView *home)
{
    BUG_ON(!buffer->abs_filename);
    set_display_filename(buffer, short_filename(buffer->abs_filename, home));
}

void buffer_update_syntax(EditorState *e, Buffer *buffer)
{
    Syntax *syn = NULL;
    if (buffer->options.syntax) {
        // Even "none" can have syntax
        syn = find_syntax(&e->syntaxes, buffer->options.filetype);
        if (!syn) {
            syn = load_syntax_by_filetype(e, buffer->options.filetype);
        }
    }
    if (syn == buffer->syn) {
        return;
    }

    buffer->syn = syn;
    if (syn) {
        // Start state of first line is constant
        PointerArray *s = &buffer->line_start_states;
        if (!s->alloc) {
            ptr_array_init(s, 64);
        }
        s->ptrs[0] = syn->start_state;
        s->count = 1;
    }

    mark_all_lines_changed(buffer);
}

static bool allow_odd_indent(uint8_t indents_bitmask)
{
    static_assert(INDENT_WIDTH_MAX == 8);
    return !!(indents_bitmask & 0x55); // 0x55 == 0b01010101
}

static int indent_len(StringView line, uint8_t indents_bitmask, bool *tab_indent)
{
    bool space_before_tab = false;
    size_t spaces = 0;
    size_t tabs = 0;
    size_t pos = 0;

    for (size_t n = line.length; pos < n; pos++) {
        switch (line.data[pos]) {
        case '\t':
            tabs++;
            if (spaces) {
                space_before_tab = true;
            }
            continue;
        case ' ':
            spaces++;
            continue;
        }
        break;
    }

    *tab_indent = false;
    if (pos == line.length) {
        return -1; // Whitespace only
    }
    if (pos == 0) {
        return 0; // Not indented
    }
    if (space_before_tab) {
        return -2; // Mixed indent
    }
    if (tabs) {
        // Tabs and possible spaces after tab for alignment
        *tab_indent = true;
        return tabs * 8;
    }
    if (line.length > spaces && line.data[spaces] == '*') {
        // '*' after indent, could be long C style comment
        if (spaces & 1 || allow_odd_indent(indents_bitmask)) {
            return spaces - 1;
        }
    }
    return spaces;
}

UNITTEST {
    bool tab;
    int len = indent_len(strview_from_cstring("    4 space"), 0, &tab);
    BUG_ON(len != 4);
    BUG_ON(tab);

    len = indent_len(strview_from_cstring("\t\t2 tab"), 0, &tab);
    BUG_ON(len != 16);
    BUG_ON(!tab);

    len = indent_len(strview_from_cstring("no indent"), 0, &tab);
    BUG_ON(len != 0);

    len = indent_len(strview_from_cstring(" \t mixed"), 0, &tab);
    BUG_ON(len != -2);

    len = indent_len(strview_from_cstring("\t  \t "), 0, &tab);
    BUG_ON(len != -1); // whitespace only

    len = indent_len(strview_from_cstring("     * 5 space"), 0, &tab);
    BUG_ON(len != 4);

    StringView line = strview_from_cstring("    * 4 space");
    len = indent_len(line, 0, &tab);
    BUG_ON(len != 4);
    len = indent_len(line, 1 << 2, &tab);
    BUG_ON(len != 3);
}

static bool detect_indent(Buffer *buffer)
{
    LocalOptions *options = &buffer->options;
    unsigned int bitset = options->detect_indent;
    BlockIter bi = block_iter(buffer);
    unsigned int tab_count = 0;
    unsigned int space_count = 0;
    int current_indent = 0;
    int counts[INDENT_WIDTH_MAX + 1] = {0};
    BUG_ON((bitset & ((1u << INDENT_WIDTH_MAX) - 1)) != bitset);

    for (size_t i = 0, j = 1; i < 200 && j > 0; i++, j = block_iter_next_line(&bi)) {
        StringView line;
        fill_line_ref(&bi, &line);
        bool tab;
        int indent = indent_len(line, bitset, &tab);
        switch (indent) {
        case -2: // Ignore mixed indent because tab width might not be 8
        case -1: // Empty line; no change in indent
            continue;
        case 0:
            current_indent = 0;
            continue;
        }

        BUG_ON(indent <= 0);
        int change = indent - current_indent;
        if (change >= 1 && change <= INDENT_WIDTH_MAX) {
            counts[change]++;
        }

        if (tab) {
            tab_count++;
        } else {
            space_count++;
        }
        current_indent = indent;
    }

    if (tab_count == 0 && space_count == 0) {
        return false;
    }

    if (tab_count > space_count) {
        options->emulate_tab = false;
        options->expand_tab = false;
        options->indent_width = options->tab_width;
        return true;
    }

    size_t m = 0;
    for (size_t i = 1; i < ARRAYLEN(counts); i++) {
        unsigned int bit = 1u << (i - 1);
        if ((bitset & bit) && counts[i] > counts[m]) {
            m = i;
        }
    }

    if (m == 0) {
        return false;
    }

    options->emulate_tab = true;
    options->expand_tab = true;
    options->indent_width = m;
    return true;
}

void buffer_setup(EditorState *e, Buffer *buffer)
{
    const char *filename = buffer->abs_filename;
    buffer->setup = true;
    buffer_detect_filetype(buffer, &e->filetypes);
    set_file_options(e, buffer);
    set_editorconfig_options(buffer);
    buffer_update_syntax(e, buffer);
    if (buffer->options.detect_indent && filename) {
        detect_indent(buffer);
    }
    sanity_check_local_options(&buffer->options);
}

void buffer_count_blocks_and_bytes(const Buffer *buffer, uintmax_t counts[2])
{
    uintmax_t blocks = 0;
    uintmax_t bytes = 0;
    Block *blk;
    block_for_each(blk, &buffer->blocks) {
        blocks += 1;
        bytes += blk->size;
    }
    counts[0] = blocks;
    counts[1] = bytes;
}

// TODO: Human-readable size (MiB/GiB/etc.) for "Bytes" and FileInfo::size
String dump_buffer(const Buffer *buffer)
{
    uintmax_t counts[2];
    buffer_count_blocks_and_bytes(buffer, counts);
    BUG_ON(counts[0] < 1);
    BUG_ON(!buffer->setup);
    String buf = string_new(1024);

    string_sprintf (
        &buf,
        "%s %s\n%s %lu\n%s %s\n%s %s\n%s %ju\n%s %zu\n%s %ju\n",
        "     Name:", buffer_filename(buffer),
        "       ID:", buffer->id,
        " Encoding:", buffer->encoding.name,
        " Filetype:", buffer->options.filetype,
        "   Blocks:", counts[0],
        "    Lines:", buffer->nl,
        "    Bytes:", counts[1]
    );

    if (
        buffer->stdout_buffer || buffer->temporary || buffer->readonly
        || buffer->locked || buffer->crlf_newlines || buffer->bom
    ) {
        string_sprintf (
            &buf,
            "    Flags:%s%s%s%s%s%s\n",
            buffer->stdout_buffer ? " STDOUT" : "",
            buffer->temporary ? " TMP" : "",
            buffer->readonly ? " RO" : "",
            buffer->locked ? " LOCKED" : "",
            buffer->crlf_newlines ? " CRLF" : "",
            buffer->bom ? " BOM" : ""
        );
    }

    if (buffer->views.count > 1) {
        string_sprintf(&buf, "    Views: %zu\n", buffer->views.count);
    }

    if (!buffer->abs_filename) {
        return buf;
    }

    const FileInfo *file = &buffer->file;
    unsigned int perms = file->mode & 07777;
    char modestr[12];
    char timestr[64];
    if (!timespec_to_str(&file->mtime, timestr, sizeof(timestr))) {
        memcpy(timestr, STRN("[error]") + 1);
    }

    string_sprintf (
        &buf,
        "\nLast stat:\n----------\n\n"
        "%s %s\n%s %s\n%s -%s (%04o)\n%s %jd\n%s %jd\n%s %ju\n%s %jd\n%s %ju\n",
        "     Path:", buffer->abs_filename,
        " Modified:", timestr,
        "     Mode:", filemode_to_str(file->mode, modestr), perms,
        "     User:", (intmax_t)file->uid,
        "    Group:", (intmax_t)file->gid,
        "     Size:", (uintmax_t)file->size,
        "   Device:", (intmax_t)file->dev,
        "    Inode:", (uintmax_t)file->ino
    );

    return buf;
}
