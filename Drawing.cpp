// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Drawing.hpp"

#include "Handles.hpp"
#include "WindowUtil.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4458)  // GDI+ headers hide class members.
#endif
#include <gdiplus.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace Varjo
{
namespace
{

Gdiplus::Color gdiplusColor(COLORREF color);
void fillRoundRect(Gdiplus::Graphics& graphics, const RoundRect& shape);

}  // namespace

void fillRect(HDC dc, const RECT& rect, COLORREF color)
{
    const COLORREF previous = SetDCBrushColor(dc, color);
    FillRect(dc, &rect, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetDCBrushColor(dc, previous);
}

void fillRoundRects(HDC dc, std::initializer_list<RoundRect> shapes)
{
    Gdiplus::Graphics graphics(dc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    for (const RoundRect& shape : shapes) {
        fillRoundRect(graphics, shape);
    }
}

void paintBehind(HDC dc, HWND child, std::initializer_list<RoundRect> shapes)
{
    const int saved = SaveDC(dc);
    const RECT childRect = windowRectInParent(child);
    ExcludeClipRect(dc, childRect.left, childRect.top, childRect.right, childRect.bottom);
    fillRoundRects(dc, shapes);
    RestoreDC(dc, saved);
}

void invalidateOutside(const RECT& area, HWND child)
{
    if (IsRectEmpty(&area)) {
        return;
    }
    const UniqueRegion region(CreateRectRgnIndirect(&area));
    const RECT childRect = windowRectInParent(child);
    const UniqueRegion inside(CreateRectRgnIndirect(&childRect));
    CombineRgn(region.get(), region.get(), inside.get(), RGN_DIFF);
    InvalidateRgn(GetParent(child), region.get(), TRUE);
}

SelectedObject::SelectedObject(HDC dc, HGDIOBJ object)
    : m_dc(dc)
    , m_previous(SelectObject(dc, object))
{
}

SelectedObject::~SelectedObject() { SelectObject(m_dc, m_previous); }

namespace
{

Gdiplus::Color gdiplusColor(COLORREF color) { return Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color)); }

void fillRoundRect(Gdiplus::Graphics& graphics, const RoundRect& shape)
{
    const auto left = static_cast<Gdiplus::REAL>(shape.rect.left);
    const auto top = static_cast<Gdiplus::REAL>(shape.rect.top);
    const auto right = static_cast<Gdiplus::REAL>(shape.rect.right);
    const auto bottom = static_cast<Gdiplus::REAL>(shape.rect.bottom);
    const Gdiplus::REAL diameter = 2 * shape.radius;
    Gdiplus::GraphicsPath path;
    path.AddArc(left, top, diameter, diameter, 180, 90);
    path.AddArc(right - diameter, top, diameter, diameter, 270, 90);
    path.AddArc(right - diameter, bottom - diameter, diameter, diameter, 0, 90);
    path.AddArc(left, bottom - diameter, diameter, diameter, 90, 90);
    path.CloseFigure();
    Gdiplus::SolidBrush brush(gdiplusColor(shape.color));
    graphics.FillPath(&brush, &path);
}

}  // namespace
}  // namespace Varjo
