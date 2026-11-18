// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Geometry.hpp"

#include <gtest/gtest.h>

using namespace Varjo;

TEST(GeometryTest, imageOfTheExactSizeShouldWin)
{
    const ImageSize sizes[] = {{18, 18}, {36, 36}};
    EXPECT_EQ(0u, chooseImage(sizes, 2, {18, 18}));
    EXPECT_EQ(1u, chooseImage(sizes, 2, {36, 36}));
}

TEST(GeometryTest, smallestLargerImageShouldBeDownscaled)
{
    const ImageSize sizes[] = {{36, 36}, {18, 18}};
    EXPECT_EQ(0u, chooseImage(sizes, 2, {27, 27}));
    EXPECT_EQ(1u, chooseImage(sizes, 2, {14, 14}));
}

TEST(GeometryTest, largestImageShouldBeUpscaledWhenNoneIsLarger)
{
    const ImageSize sizes[] = {{18, 18}, {36, 36}};
    EXPECT_EQ(1u, chooseImage(sizes, 2, {45, 45}));
}

TEST(GeometryTest, imageShouldBeLargeInBothDimensions)
{
    const ImageSize sizes[] = {{163, 32}, {326, 64}};
    EXPECT_EQ(1u, chooseImage(sizes, 2, {163, 33}));
}

TEST(GeometryTest, emptyProgressBarShouldHaveNoFill)
{
    // At 0 percent only the end column on the left has the color of the filled part.
    EXPECT_EQ(0, progressFillWidth(1, 432));
    EXPECT_EQ(0, progressFillWidth(0, 432));
}

TEST(GeometryTest, fullProgressBarShouldBeFilledToTheRightEnd)
{
    // At 100 percent the end column on the right keeps the color of the rest.
    EXPECT_EQ(432, progressFillWidth(431, 432));
    EXPECT_EQ(432, progressFillWidth(432, 432));
}

TEST(GeometryTest, progressBarFillShouldScaleToTheWholeWidth)
{
    // thmutil fills 215 of its 430 fillable columns at 50 percent.
    EXPECT_EQ(216, progressFillWidth(216, 432));
}

TEST(GeometryTest, narrowProgressBarShouldHaveNoFill) { EXPECT_EQ(0, progressFillWidth(2, 2)); }

TEST(GeometryTest, contentShouldBeCenteredBetweenThePaddings)
{
    // The Accept and Install button is 163 pixels wide, and its label of 113 pixels fills the space
    // between the paddings.
    EXPECT_EQ(12, contentLeft(163, 18 + 4 + 113, 12, 16));
    EXPECT_EQ(22, contentLeft(163, 18 + 4 + 93, 12, 16));
}
