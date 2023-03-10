#include "test.h"
#include "editor.h"
#include "frame.h"
#include "window.h"

static void test_frame_resize(TestContext *ctx)
{
    EditorState e = {.window = new_window(&e)};
    ASSERT_NONNULL(e.window);
    e.root_frame = new_root_frame(e.window);
    ASSERT_NONNULL(e.root_frame);

    Frame *root = e.root_frame;
    frame_set_size(root, 80, 23);
    //update_window_coordinates(root);
    // TODO: also check window dimensions
    EXPECT_EQ(root->w, 80);
    EXPECT_EQ(root->h, 23);

    Frame *frame = frame_split_root(&e, false, false);
    root = e.root_frame;
    ASSERT_NONNULL(frame);
    ASSERT_NONNULL(frame->window);
    ASSERT_NONNULL(root);
    ASSERT_PTREQ(frame->parent, root);
    EXPECT_EQ(frame->w, 40);
    EXPECT_EQ(frame->h, 23);
    EXPECT_EQ(frame->frames.count, 0);
    EXPECT_FALSE(frame->vertical);
    EXPECT_TRUE(frame->equal_size);
    EXPECT_TRUE(root->equal_size);

    frame_add_to_size(frame, RESIZE_DIRECTION_AUTO, 1);
    EXPECT_EQ(frame->w, 41);
    EXPECT_EQ(frame->h, 23);
    EXPECT_TRUE(frame->equal_size);
    EXPECT_FALSE(root->equal_size);

    frame_add_to_size(frame, RESIZE_DIRECTION_AUTO, 900);
    EXPECT_EQ(frame->w, 72);
    EXPECT_EQ(frame->h, 23);

    frame_add_to_size(frame, RESIZE_DIRECTION_AUTO, -900);
    EXPECT_EQ(frame->w, 8);
    EXPECT_EQ(frame->h, 23);

    frame_resize(frame, RESIZE_DIRECTION_AUTO, 63);
    EXPECT_EQ(frame->w, 63);
    EXPECT_EQ(frame->h, 23);

    frame_resize(frame, RESIZE_DIRECTION_HORIZONTAL, 10);
    EXPECT_EQ(frame->w, 10);
    EXPECT_EQ(frame->h, 23);

    frame_resize(frame, RESIZE_DIRECTION_VERTICAL, 55);
    EXPECT_EQ(frame->w, 10);
    EXPECT_EQ(frame->h, 23);

    EXPECT_FALSE(root->equal_size);
    frame_equalize_sizes(root);
    EXPECT_TRUE(root->equal_size);
    EXPECT_EQ(frame->w, 40);
    EXPECT_EQ(frame->h, 23);

    EXPECT_PTREQ(root, e.root_frame);
    EXPECT_PTREQ(root, frame->parent);
    frame_remove(&e, e.root_frame);
}

static const TestEntry tests[] = {
    TEST(test_frame_resize),
};

const TestGroup frame_tests = TEST_GROUP(tests);
