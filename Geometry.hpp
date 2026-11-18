// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <cstddef>
#include <cstdint>

// The sizes and positions with which the setup draws its images. Nothing here needs a window.
namespace Varjo
{

struct ImageSize {
    int width;
    int height;
};

// Returns the index of the image that thmutil draws into a rectangle of the target size. That is
// the image of that size, or else the smallest image that is at least as large in both
// dimensions, or else the largest image. sizes must not be empty.
constexpr size_t chooseImage(const ImageSize* sizes, size_t count, ImageSize target)
{
    size_t best = 0;
    bool haveLarger = false;
    int64_t bestPixels = 0;
    for (size_t i = 0; i < count; ++i) {
        const ImageSize& size = sizes[i];
        const int64_t pixels = static_cast<int64_t>(size.width) * size.height;
        if (size.width == target.width && size.height == target.height) {
            return i;
        }
        if (size.width >= target.width && size.height >= target.height) {
            if (!haveLarger || pixels < bestPixels) {
                best = i;
                bestPixels = pixels;
                haveLarger = true;
            }
        } else if (!haveLarger && pixels > bestPixels) {
            best = i;
            bestPixels = pixels;
        }
    }
    return best;
}

// Returns the width of the filled part of a progress bar of width pixels from the image that
// thmutil draws, which begins with filledColumns columns in the color of the filled part. thmutil
// draws a column at each end and fills the columns between them. The end column on the left has
// the color of the filled part, and the end column on the right the color of the rest.
constexpr int progressFillWidth(int filledColumns, int width)
{
    const int fillable = width - 2;
    if (fillable <= 0 || filledColumns <= 1) {
        return 0;
    }
    const int filled = filledColumns - 1 < fillable ? filledColumns - 1 : fillable;
    return (filled * width + fillable / 2) / fillable;
}

// Returns the left edge of the content of a button, which is its icon and its label. The content is
// centered between the paddings. All lengths are in the same unit.
constexpr int contentLeft(int buttonWidth, int contentWidth, int leftPadding, int rightPadding)
{
    return leftPadding + (buttonWidth - leftPadding - rightPadding - contentWidth) / 2;
}

}  // namespace Varjo
