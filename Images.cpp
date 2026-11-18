// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Images.hpp"

#include "Geometry.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4458)  // GDI+ headers hide class members.
#endif
#include <gdiplus.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <dutil.h>

#include <IBootstrapperEngine.h>
#include <balutil.h>

namespace Varjo
{
namespace
{

std::unique_ptr<Gdiplus::Bitmap> loadBitmap(const std::wstring& path);

}  // namespace

ImageCache::ImageCache(HMODULE module)
{
    wchar_t path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(module, path, ARRAYSIZE(path));
    m_directory.assign(path, length);
    m_directory.erase(m_directory.find_last_of(L'\\') + 1);
}

ImageCache::~ImageCache() = default;

void ImageCache::draw(HDC dc, std::wstring_view name, const RECT& rect)
{
    const Image& image = findImage(name);
    std::array<ImageSize, 2> sizes = {};
    std::array<Gdiplus::Bitmap*, 2> bitmaps = {};
    size_t count = 0;
    for (const auto& bitmap : image.bitmaps) {
        if (bitmap) {
            sizes[count] = {static_cast<int>(bitmap->GetWidth()), static_cast<int>(bitmap->GetHeight())};
            bitmaps[count] = bitmap.get();
            ++count;
        }
    }
    if (count == 0) {
        return;
    }
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    Gdiplus::Bitmap* const bitmap = bitmaps[chooseImage(sizes.data(), count, {width, height})];

    // These are the settings with which thmutil draws the images of the theme.
    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
    Gdiplus::ImageAttributes attributes;
    attributes.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
    graphics.DrawImage(bitmap, Gdiplus::Rect(rect.left, rect.top, width, height), 0, 0, static_cast<INT>(bitmap->GetWidth()),
        static_cast<INT>(bitmap->GetHeight()), Gdiplus::UnitPixel, &attributes);
}

const ImageCache::Image& ImageCache::findImage(std::wstring_view name)
{
    auto found = m_images.find(name);
    if (found == m_images.end()) {
        const std::wstring base = m_directory + std::wstring(name);
        Image image;
        image.bitmaps[0] = loadBitmap(base + L".png");
        image.bitmaps[1] = loadBitmap(base + L"-2x.png");
        found = m_images.emplace(std::wstring(name), std::move(image)).first;
    }
    return found->second;
}

namespace
{

std::unique_ptr<Gdiplus::Bitmap> loadBitmap(const std::wstring& path)
{
    std::unique_ptr<Gdiplus::Bitmap> bitmap(Gdiplus::Bitmap::FromFile(path.c_str()));
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok) {
        BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "VARJO: The setup has no image %ls.", path.c_str());
        return nullptr;
    }
    return bitmap;
}

}  // namespace
}  // namespace Varjo
