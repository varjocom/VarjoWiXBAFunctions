// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <initializer_list>

// Drawing that the controls of the setup share. Lengths are physical pixels. GDI+ must be running.
namespace Varjo
{

void fillRect(HDC dc, const RECT& rect, COLORREF color);

// A rectangle with rounded corners. Each pixel covers the unit square right and below its
// coordinates, so the edges of rect stay sharp.
struct RoundRect {
    RECT rect;
    float radius;
    COLORREF color;
};

// Fills the shapes with antialiasing, each over the ones before it.
void fillRoundRects(HDC dc, std::initializer_list<RoundRect> shapes);

// Fills the shapes on the DC of the parent of child, outside the rectangle of child.
void paintBehind(HDC dc, HWND child, std::initializer_list<RoundRect> shapes);

// Repaints the part of area outside child, in the client coordinates of the parent of child. The
// window of the setup has no WS_CLIPCHILDREN, so erasing the area of the child would erase it.
void invalidateOutside(const RECT& area, HWND child);

// Selects an object into a DC, and selects the earlier object back when it goes away.
class SelectedObject
{
public:
    SelectedObject(HDC dc, HGDIOBJ object);
    ~SelectedObject();
    SelectedObject(const SelectedObject&) = delete;
    SelectedObject& operator=(const SelectedObject&) = delete;

private:
    HDC m_dc;
    HGDIOBJ m_previous;
};

}  // namespace Varjo
