#include "frame.h"
#include "editor.h"
#include "util/xmalloc.h"
#include "window.h"

enum {
    WINDOW_MIN_WIDTH = 8,
    WINDOW_MIN_HEIGHT = 3,
};

// Recursion is bounded by the number of descendant frames, which is
// typically not more than 5 or so
// NOLINTBEGIN(misc-no-recursion)

static void sanity_check_frame(const Frame *frame)
{
    bool has_window = !!frame->window;
    bool has_frames = frame->frames.count > 0;
    if (has_window == has_frames) {
        BUG("frames must contain a window or subframe(s), but never both");
    }
    BUG_ON(has_window && frame != frame->window->frame);
}

static int get_min_w(const Frame *frame)
{
    if (frame->window) {
        return WINDOW_MIN_WIDTH;
    }

    void **subframes = frame->frames.ptrs;
    size_t count = frame->frames.count;
    if (!frame->vertical) {
        int w = count - 1; // Separators
        for (size_t i = 0; i < count; i++) {
            w += get_min_w(subframes[i]);
        }
        return w;
    }

    int max = 0;
    for (size_t i = 0; i < count; i++) {
        int w = get_min_w(subframes[i]);
        max = MAX(w, max);
    }
    return max;
}

static int get_min_h(const Frame *frame)
{
    if (frame->window) {
        return WINDOW_MIN_HEIGHT;
    }

    void **subframes = frame->frames.ptrs;
    size_t count = frame->frames.count;
    if (frame->vertical) {
        int h = 0;
        for (size_t i = 0; i < count; i++) {
            h += get_min_h(subframes[i]);
        }
        return h;
    }

    int max = 0;
    for (size_t i = 0; i < count; i++) {
        int h = get_min_h(subframes[i]);
        max = MAX(h, max);
    }
    return max;
}

// Get parent frame and assert non-NULL, for use in contexts where
// `frame` may not be the root frame
static Frame *frame_must_get_parent(const Frame *frame)
{
    Frame *parent = frame->parent;
    BUG_ON(!parent);
    return parent;
}

static int get_min(const Frame *frame)
{
    const Frame *parent = frame_must_get_parent(frame);
    return parent->vertical ? get_min_h(frame) : get_min_w(frame);
}

static int get_size(const Frame *frame)
{
    const Frame *parent = frame_must_get_parent(frame);
    return parent->vertical ? frame->h : frame->w;
}

static int get_container_size(const Frame *frame)
{
    return frame->vertical ? frame->h : frame->w;
}

static void set_size(Frame *frame, int size)
{
    const Frame *parent = frame_must_get_parent(frame);
    bool vertical = parent->vertical;
    int w = vertical ? parent->w : size;
    int h = vertical ? size : parent->h;
    frame_set_size(frame, w, h);
}

static void divide_equally(const Frame *frame)
{
    void **ptrs = frame->frames.ptrs;
    size_t count = frame->frames.count;
    BUG_ON(count == 0);
    BUG_ON(!ptrs);

    int *min = xmallocarray(count, sizeof(int));
    for (size_t i = 0; i < count; i++) {
        min[i] = get_min(ptrs[i]);
    }

    int *size = xcalloc(count, sizeof(int));
    int s = get_container_size(frame);
    int q, r, used;
    size_t n = count;

    // Consume q and r as equally as possible
    do {
        used = 0;
        q = s / n;
        r = s % n;
        for (size_t i = 0; i < count; i++) {
            if (size[i] == 0 && min[i] > q) {
                size[i] = min[i];
                used += min[i];
                n--;
            }
        }
        s -= used;
    } while (used && n > 0);

    for (size_t i = 0; i < count; i++) {
        Frame *c = ptrs[i];
        if (size[i] == 0) {
            size[i] = q + (r-- > 0);
        }
        set_size(c, size[i]);
    }

    free(size);
    free(min);
}

static void fix_size(const Frame *frame)
{
    void **ptrs = frame->frames.ptrs;
    size_t count = frame->frames.count;
    int *size = xmallocarray(count, sizeof(int));
    int *min = xmallocarray(count, sizeof(int));
    int total = 0;

    for (size_t i = 0; i < count; i++) {
        const Frame *c = ptrs[i];
        min[i] = get_min(c);
        size[i] = MAX(get_size(c), min[i]);
        total += size[i];
    }

    int s = get_container_size(frame);
    if (total > s) {
        int n = total - s;
        for (ssize_t i = count - 1; n > 0 && i >= 0; i--) {
            int new_size = MAX(size[i] - n, min[i]);
            n -= size[i] - new_size;
            size[i] = new_size;
        }
    } else {
        size[count - 1] += s - total;
    }

    for (size_t i = 0; i < count; i++) {
        set_size(ptrs[i], size[i]);
    }

    free(size);
    free(min);
}

static void add_to_sibling_size(Frame *frame, int count)
{
    const Frame *parent = frame_must_get_parent(frame);
    const PointerArray *pframes = &parent->frames;
    size_t idx = ptr_array_xindex(pframes, frame);
    bool last = (idx == pframes->count - 1);
    frame = pframes->ptrs[last ? idx - 1 : idx + 1];
    set_size(frame, get_size(frame) + count);
}

static int sub(Frame *frame, int count)
{
    int min = get_min(frame);
    int old = get_size(frame);
    int new = MAX(min, old - count);
    if (new != old) {
        set_size(frame, new);
    }
    return count - (old - new);
}

static void subtract_from_sibling_size(const Frame *frame, int count)
{
    const Frame *parent = frame_must_get_parent(frame);
    const PointerArray *pframes = &parent->frames;
    size_t idx = ptr_array_xindex(pframes, frame);
    void **ptrs = pframes->ptrs;

    for (size_t i = idx + 1, n = pframes->count; i < n; i++) {
        count = sub(ptrs[i], count);
        if (count == 0) {
            return;
        }
    }

    for (size_t i = idx; i > 0; i--) {
        count = sub(ptrs[i - 1], count);
        if (count == 0) {
            return;
        }
    }
}

static void resize_to(Frame *frame, int size)
{
    const Frame *parent = frame_must_get_parent(frame);
    size_t count = parent->frames.count;
    BUG_ON(count == 0);

    int total = parent->vertical ? parent->h : parent->w;
    int min = get_min(frame);
    int max = total - ((count - 1) * min);
    max = MAX(min, max);
    size = CLAMP(size, min, max);

    int change = size - get_size(frame);
    if (change == 0) {
        return;
    }

    set_size(frame, size);
    if (change < 0) {
        add_to_sibling_size(frame, -change);
    } else {
        subtract_from_sibling_size(frame, change);
    }
}

static bool rightmost_frame(const Frame *frame)
{
    const Frame *parent = frame->parent;
    if (!parent) {
        return true;
    }
    if (!parent->vertical) {
        if (frame != parent->frames.ptrs[parent->frames.count - 1]) {
            return false;
        }
    }
    return rightmost_frame(parent);
}

static Frame *new_frame(void)
{
    Frame *frame = xcalloc1(sizeof(*frame));
    frame->equal_size = true;
    return frame;
}

static Frame *add_frame(Frame *parent, Window *window, size_t idx)
{
    Frame *frame = new_frame();
    frame->parent = parent;
    frame->window = window;
    window->frame = frame;
    if (parent) {
        BUG_ON(idx > parent->frames.count);
        ptr_array_insert(&parent->frames, frame, idx);
        parent->window = NULL;
    }
    return frame;
}

Frame *new_root_frame(Window *window)
{
    return add_frame(NULL, window, 0);
}

static Frame *find_resizable(Frame *frame, ResizeDirection dir)
{
    if (dir == RESIZE_DIRECTION_AUTO) {
        return frame;
    }

    while (frame->parent) {
        if (dir == RESIZE_DIRECTION_VERTICAL && frame->parent->vertical) {
            return frame;
        }
        if (dir == RESIZE_DIRECTION_HORIZONTAL && !frame->parent->vertical) {
            return frame;
        }
        frame = frame->parent;
    }
    return NULL;
}

void frame_set_size(Frame *frame, int w, int h)
{
    int min_w = get_min_w(frame);
    int min_h = get_min_h(frame);
    w = MAX(w, min_w);
    h = MAX(h, min_h);
    frame->w = w;
    frame->h = h;

    if (frame->window) {
        w -= rightmost_frame(frame) ? 0 : 1; // Separator
        window_set_size(frame->window, w, h);
        return;
    }

    if (frame->equal_size) {
        divide_equally(frame);
    } else {
        fix_size(frame);
    }
}

void frame_equalize_sizes(Frame *parent)
{
    parent->equal_size = true;
    divide_equally(parent);
    update_window_coordinates(parent);
}

void frame_resize(Frame *frame, ResizeDirection dir, int size)
{
    frame = find_resizable(frame, dir);
    if (!frame) {
        return;
    }

    Frame *parent = frame_must_get_parent(frame);
    parent->equal_size = false;
    resize_to(frame, size);
    update_window_coordinates(parent);
}

void frame_add_to_size(Frame *frame, ResizeDirection dir, int amount)
{
    frame_resize(frame, dir, get_size(frame) + amount);
}

static void update_frame_coordinates(const Frame *frame, int x, int y)
{
    if (frame->window) {
        window_set_coordinates(frame->window, x, y);
        return;
    }

    void **ptrs = frame->frames.ptrs;
    for (size_t i = 0, n = frame->frames.count; i < n; i++) {
        const Frame *c = ptrs[i];
        update_frame_coordinates(c, x, y);
        if (frame->vertical) {
            y += c->h;
        } else {
            x += c->w;
        }
    }
}

static Frame *get_root_frame(Frame *frame)
{
    BUG_ON(!frame);
    while (frame->parent) {
        frame = frame->parent;
    }
    return frame;
}

void update_window_coordinates(Frame *frame)
{
    update_frame_coordinates(get_root_frame(frame), 0, 0);
}

Frame *frame_split(Window *window, bool vertical, bool before)
{
    Frame *frame = window->frame;
    Frame *parent = frame->parent;
    if (!parent || parent->vertical != vertical) {
        // Reparent window
        frame->vertical = vertical;
        add_frame(frame, window, 0);
        parent = frame;
    }

    size_t idx = ptr_array_xindex(&parent->frames, window->frame);
    idx += before ? 0 : 1;
    frame = add_frame(parent, new_window(window->editor), idx);
    parent->equal_size = true;

    // Recalculate
    frame_set_size(parent, parent->w, parent->h);
    update_window_coordinates(parent);
    return frame;
}

// Doesn't really split root but adds new frame between root and its contents
Frame *frame_split_root(EditorState *e, bool vertical, bool before)
{
    Frame *old_root = e->root_frame;
    Frame *new_root = new_frame();
    ptr_array_append(&new_root->frames, old_root);
    old_root->parent = new_root;
    new_root->vertical = vertical;
    e->root_frame = new_root;

    Frame *frame = add_frame(new_root, new_window(e), before ? 0 : 1);
    frame_set_size(new_root, old_root->w, old_root->h);
    update_window_coordinates(new_root);
    return frame;
}

// NOTE: does not remove frame from frame->parent->frames
static void free_frame(Frame *frame)
{
    frame->parent = NULL;
    ptr_array_free_cb(&frame->frames, FREE_FUNC(free_frame));

    if (frame->window) {
        window_free(frame->window);
        frame->window = NULL;
    }

    free(frame);
}

void frame_remove(EditorState *e, Frame *frame)
{
    Frame *parent = frame->parent;
    if (!parent) {
        free_frame(frame);
        return;
    }

    ptr_array_remove(&parent->frames, frame);
    free_frame(frame);

    if (parent->frames.count == 1) {
        // Replace parent with the only child frame
        Frame *gp = parent->parent;
        Frame *c = parent->frames.ptrs[0];
        c->parent = gp;
        c->w = parent->w;
        c->h = parent->h;
        if (gp) {
            size_t idx = ptr_array_xindex(&gp->frames, parent);
            gp->frames.ptrs[idx] = c;
        } else {
            e->root_frame = c;
        }
        ptr_array_free_array(&parent->frames);
        free(parent);
        parent = c;
    }

    // Recalculate
    frame_set_size(parent, parent->w, parent->h);
    update_window_coordinates(parent);
}

void dump_frame(const Frame *frame, size_t level, String *str)
{
    sanity_check_frame(frame);
    string_append_memset(str, ' ', level * 4);
    string_sprintf(str, "%dx%d", frame->w, frame->h);

    const Window *w = frame->window;
    if (w) {
        const char *name = buffer_filename(w->view->buffer);
        string_append_byte(str, '\n');
        string_append_memset(str, ' ', (level + 1) * 4);
        string_sprintf(str, "%d,%d %dx%d %s\n", w->x, w->y, w->w, w->h, name);
        return;
    }

    string_append_cstring(str, frame->vertical ? " V" : " H");
    string_append_cstring(str, frame->equal_size ? "\n" : " !\n");

    for (size_t i = 0, n = frame->frames.count; i < n; i++) {
        const Frame *c = frame->frames.ptrs[i];
        dump_frame(c, level + 1, str);
    }
}

#if DEBUG_ASSERTIONS_ENABLED
void frame_debug(const Frame *frame)
{
    sanity_check_frame(frame);
    for (size_t i = 0, n = frame->frames.count; i < n; i++) {
        const Frame *c = frame->frames.ptrs[i];
        BUG_ON(c->parent != frame);
        frame_debug(c);
    }
}
#endif

// NOLINTEND(misc-no-recursion)
