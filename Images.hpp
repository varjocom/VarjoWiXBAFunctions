// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace Gdiplus
{
class Bitmap;
}  // namespace Gdiplus

namespace Varjo
{

// The images that BAFunctions draws itself. They are PNG files of the theme in the directory of the
// DLL, where the setup extracts the theme. Each image has a file at 96 DPI, as in
// icon-shield-hover.png, and one at twice the size, as in icon-shield-hover-2x.png. The cache loads
// an image when it first draws it. GDI+ must run as long as the cache exists, and the cache is used
// on the thread of the window only.
class ImageCache
{
public:
    // Takes the directory of the images from the path of module.
    explicit ImageCache(HMODULE module);
    ~ImageCache();
    ImageCache(const ImageCache&) = delete;
    ImageCache& operator=(const ImageCache&) = delete;

    // Draws the image with the name, as in icon-shield-hover, stretched into rect. Draws nothing when
    // the image is missing, and logs the missing file once.
    void draw(HDC dc, std::wstring_view name, const RECT& rect);

private:
    // The files of one image, at 96 DPI and at 192 DPI. A missing file leaves its bitmap empty.
    struct Image {
        std::array<std::unique_ptr<Gdiplus::Bitmap>, 2> bitmaps;
    };

    const Image& findImage(std::wstring_view name);

    std::wstring m_directory;
    std::map<std::wstring, Image, std::less<>> m_images;
};

}  // namespace Varjo
