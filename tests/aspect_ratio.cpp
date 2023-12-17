#include "gtest/gtest.h"
extern "C"
{
#include "dlibc.h"
}

TEST(AspectRatio, Works)
{
    EXPECT_EQ(d_aspect_ratio_comparison(1280, 720), 1);
    EXPECT_EQ(d_aspect_ratio_comparison(1280, 800), 0);
    EXPECT_EQ(d_aspect_ratio_comparison(1280, 960), -1);

    uint32_t skip_x, skip_y;

    d_get_screen_params(320, 240, &skip_x, &skip_y);
    EXPECT_EQ(skip_y, 20);
    EXPECT_EQ(skip_x, 0);

    d_get_screen_params(356, 200, &skip_x, &skip_y);
    EXPECT_EQ(skip_y, 0);
    EXPECT_EQ(skip_x, 18);
}