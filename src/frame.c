#include "frame.h"
#include "window.h"

Frame *root_frame;

static int get_min_w(Frame *f)
{
    if (f->window) {
        return 8;
    }

    if (f->vertical) {
        int max = 0;
        for (int i = 0; i < f->frames.count; i++) {
            int w = get_min_w(f->frames.ptrs[i]);
            if (w > max) {
                max = w;
            }
        }
        return max;
    } else {
        int w = f->frames.count - 1; // Separators
        for (int i = 0; i < f->frames.count; i++) {
            w += get_min_w(f->frames.ptrs[i]);
        }
        return w;
    }
}

static int get_min_h(Frame *f)
{
    if (f->window) {
        return 3;
    }

    if (!f->vertical) {
        int max = 0;
        for (int i = 0; i < f->frames.count; i++) {
            int h = get_min_h(f->frames.ptrs[i]);
            if (h > max) {
                max = h;
            }
        }
        return max;
    } else {
        int h = 0; // No separators
        for (int i = 0; i < f->frames.count; i++) {
            h += get_min_h(f->frames.ptrs[i]);
        }
        return h;
    }
}

static int get_min(Frame *f)
{
    if (f->parent->vertical) {
        return get_min_h(f);
    }
    return get_min_w(f);
}

static int get_size(Frame *f)
{
    if (f->parent->vertical) {
        return f->h;
    }
    return f->w;
}

static int get_container_size(Frame *f)
{
    if (f->vertical) {
        return f->h;
    }
    return f->w;
}

static void set_size(Frame *f, int size)
{
    if (f->parent->vertical) {
        set_frame_size(f, f->parent->w, size);
    } else {
        set_frame_size(f, size, f->parent->h);
    }
}

static void divide_equally(Frame *f)
{
    int q, r, s, n, used, count = f->frames.count;
    int *size, *min;

    size = xnew0(int, count);
    min = xnew(int, count);
    for (int i = 0; i < count; i++) {
        min[i] = get_min(f->frames.ptrs[i]);
    }

    s = get_container_size(f);

    // Consume q and r as equally as possible
    n = count;
    do {
        used = 0;
        q = s / n;
        r = s % n;
        for (int i = 0; i < count; i++) {
            if (size[i] == 0 && min[i] > q) {
                size[i] = min[i];
                used += min[i];
                n--;
            }
        }
        s -= used;
    } while (used && n > 0);

    for (int i = 0; i < count; i++) {
        Frame *c = f->frames.ptrs[i];

        if (size[i] == 0) {
            size[i] = q + (r-- > 0);
        }

        set_size(c, size[i]);
    }

    free(size);
    free(min);
}

static void fix_size(Frame *f)
{
    int s, total, count = f->frames.count;
    int *size, *min;

    size = xnew0(int, count);
    min = xnew(int, count);
    total = 0;
    for (int i = 0; i < count; i++) {
        Frame *c = f->frames.ptrs[i];
        min[i] = get_min(c);
        size[i] = get_size(c);
        if (size[i] < min[i]) {
            size[i] = min[i];
        }
        total += size[i];
    }

    s = get_container_size(f);
    if (total > s) {
        int n = total - s;

        for (int i = count - 1; n > 0 && i >= 0; i--) {
            int new_size = size[i] - n;
            if (new_size < min[i]) {
                new_size = min[i];
            }
            n -= size[i] - new_size;
            size[i] = new_size;
        }
    } else {
        size[count - 1] += s - total;
    }

    for (int i = 0; i < count; i++) {
        set_size(f->frames.ptrs[i], size[i]);
    }

    free(size);
    free(min);
}

static void add_to_sibling_size(Frame *f, int count)
{
    Frame *parent = f->parent;
    int idx = ptr_array_idx(&parent->frames, f);

    if (idx == parent->frames.count - 1) {
        f = parent->frames.ptrs[idx - 1];
    } else {
        f = parent->frames.ptrs[idx + 1];
    }
    set_size(f, get_size(f) + count);
}

static int sub(Frame *f, int count)
{
    int min = get_min(f);
    int old = get_size(f);
    int new = old - count;
    if (new < min) {
        new = min;
    }
    if (new != old) {
        set_size(f, new);
    }
    return count - (old - new);
}

static void subtract_from_sibling_size(Frame *f, int count)
{
    Frame *parent = f->parent;
    int idx = ptr_array_idx(&parent->frames, f);

    for (int i = idx + 1; i < parent->frames.count; i++) {
        count = sub(parent->frames.ptrs[i], count);
        if (count == 0) {
            return;
        }
    }
    for (int i = idx - 1; i >= 0; i--) {
        count = sub(parent->frames.ptrs[i], count);
        if (count == 0) {
            return;
        }
    }
}

static void resize_to(Frame *f, int size)
{
    Frame *parent = f->parent;
    int total = parent->vertical ? parent->h : parent->w;
    int count = parent->frames.count;
    int min = get_min(f);
    int max = total - (count - 1) * min;
    int change;

    if (max < min) {
        max = min;
    }
    if (size < min) {
        size = min;
    }
    if (size > max) {
        size = max;
    }

    change = size - get_size(f);
    if (change == 0) {
        return;
    }

    set_size(f, size);
    if (change < 0) {
        add_to_sibling_size(f, -change);
    } else {
        subtract_from_sibling_size(f, change);
    }
}

static bool rightmost_frame(Frame *f)
{
    Frame *parent = f->parent;

    if (parent == NULL) {
        return true;
    }
    if (!parent->vertical) {
        if (f != parent->frames.ptrs[parent->frames.count - 1]) {
            return false;
        }
    }
    return rightmost_frame(parent);
}

static Frame *new_frame(void)
{
    Frame *f = xnew0(Frame, 1);

    f->equal_size = true;
    return f;
}

static Frame *add_frame(Frame *parent, Window *w, int idx)
{
    Frame *f = new_frame();

    f->parent = parent;
    f->window = w;
    w->frame = f;
    if (parent != NULL) {
        BUG_ON(idx > parent->frames.count);
        ptr_array_insert(&parent->frames, f, idx);
        parent->window = NULL;
    }
    return f;
}

Frame *new_root_frame(Window *w)
{
    return add_frame(NULL, w, 0);
}

static Frame *find_resizable(Frame *f, ResizeDirection dir)
{
    if (dir == RESIZE_DIRECTION_AUTO) {
        return f;
    }

    while (f->parent) {
        if (dir == RESIZE_DIRECTION_VERTICAL && f->parent->vertical) {
            return f;
        }
        if (dir == RESIZE_DIRECTION_HORIZONTAL && !f->parent->vertical) {
            return f;
        }
        f = f->parent;
    }
    return NULL;
}

void set_frame_size(Frame *f, int w, int h)
{
    int min_w = get_min_w(f);
    int min_h = get_min_h(f);

    if (w < min_w) {
        w = min_w;
    }
    if (h < min_h) {
        h = min_h;
    }
    f->w = w;
    f->h = h;
    if (f->window) {
        if (!rightmost_frame(f)) {
            w--; // Separator
        }
        set_window_size(f->window, w, h);
        return;
    }

    if (f->equal_size) {
        divide_equally(f);
    } else {
        fix_size(f);
    }
}

void equalize_frame_sizes(Frame *parent)
{
    parent->equal_size = true;
    divide_equally(parent);
    update_window_coordinates();
}

void add_to_frame_size(Frame *f, ResizeDirection dir, int amount)
{
    f = find_resizable(f, dir);
    if (f == NULL) {
        return;
    }

    f->parent->equal_size = false;
    if (f->parent->vertical) {
        resize_to(f, f->h + amount);
    } else {
        resize_to(f, f->w + amount);
    }
    update_window_coordinates();
}

void resize_frame(Frame *f, ResizeDirection dir, int size)
{
    f = find_resizable(f, dir);
    if (f == NULL) {
        return;
    }

    f->parent->equal_size = false;
    resize_to(f, size);
    update_window_coordinates();
}

static void update_frame_coordinates(Frame *f, int x, int y)
{
    if (f->window) {
        set_window_coordinates(f->window, x, y);
        return;
    }

    for (int i = 0; i < f->frames.count; i++) {
        Frame *c = f->frames.ptrs[i];
        update_frame_coordinates(c, x, y);
        if (f->vertical) {
            y += c->h;
        } else {
            x += c->w;
        }
    }
}

void update_window_coordinates(void)
{
    update_frame_coordinates(root_frame, 0, 0);
}

Frame *split_frame(Window *w, bool vertical, bool before)
{
    Frame *f, *parent;
    Window *neww;
    int idx;

    f = w->frame;
    parent = f->parent;
    if (parent == NULL || parent->vertical != vertical) {
        // Reparent w
        f->vertical = vertical;
        add_frame(f, w, 0);
        parent = f;
    }

    idx = ptr_array_idx(&parent->frames, w->frame);
    if (!before) {
        idx++;
    }
    neww = new_window();
    f = add_frame(parent, neww, idx);
    parent->equal_size = true;

    // Recalculate
    set_frame_size(parent, parent->w, parent->h);
    update_window_coordinates();
    return f;
}

// Doesn't really split root but adds new frame between root and its contents
Frame *split_root(bool vertical, bool before)
{
    Frame *new_root, *f;

    new_root = new_frame();
    new_root->vertical = vertical;
    f = new_frame();
    f->parent = new_root;
    f->window = new_window();
    f->window->frame = f;
    if (before) {
        ptr_array_add(&new_root->frames, f);
        ptr_array_add(&new_root->frames, root_frame);
    } else {
        ptr_array_add(&new_root->frames, root_frame);
        ptr_array_add(&new_root->frames, f);
    }

    root_frame->parent = new_root;
    set_frame_size(new_root, root_frame->w, root_frame->h);
    root_frame = new_root;
    update_window_coordinates();
    return f;
}

// NOTE: does not remove f from f->parent->frames
static void free_frame(Frame *f)
{
    f->parent = NULL;
    ptr_array_free_cb(&f->frames, FREE_FUNC(free_frame));
    if (f->window != NULL) {
        window_free(f->window);
        f->window = NULL;
    }
    free(f);
}

void remove_frame(Frame *f)
{
    Frame *parent = f->parent;

    if (parent == NULL) {
        free_frame(f);
        return;
    }
    ptr_array_remove(&parent->frames, f);
    free_frame(f);

    if (parent->frames.count == 1) {
        // Replace parent with the only child frame
        Frame *gp = parent->parent;
        Frame *c = parent->frames.ptrs[0];

        c->parent = gp;
        c->w = parent->w;
        c->h = parent->h;
        if (gp) {
            long idx = ptr_array_idx(&gp->frames, parent);
            gp->frames.ptrs[idx] = c;
        } else {
            root_frame = c;
        }
        free(parent->frames.ptrs);
        free(parent);
        parent = c;
    }

    // Recalculate
    set_frame_size(parent, parent->w, parent->h);
    update_window_coordinates();
}

#ifdef DEBUG_FRAMES

static void debug_frame(Frame *f, int level)
{
    d_print (
        "%*s%dx%d %d %d %zu\n",
        level * 4, "",
        f->w, f->h,
        f->vertical, f->equal_size,
        f->frames.count
    );

    if (f->window) {
        d_print (
            "%*swindow %d,%d %dx%d\n",
            (level + 1) * 4, "",
            f->window->x,
            f->window->y,
            f->window->w,
            f->window->h
        );
    }

    BUG_ON(f->window && f->frames.count);
    if (f->window) {
        BUG_ON(f != f->window->frame);
    }

    for (int i = 0; i < f->frames.count; i++) {
        Frame *c = f->frames.ptrs[i];
        BUG_ON(c->parent != f);
        debug_frame(c, level + 1);
    }
}

void debug_frames(void)
{
    debug_frame(root_frame, 0);
}

#else

void debug_frames(void) {}

#endif
