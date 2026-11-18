// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Dimmer.hpp"

#include "WindowUtil.hpp"

namespace Varjo
{
namespace
{

constexpr wchar_t c_windowClassName[] = L"VarjoDimmer";
// 75 % of full opacity.
constexpr BYTE c_alpha = 191;

LRESULT CALLBACK dimmerProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

}  // namespace

Dimmer::Dimmer(HINSTANCE instance, HWND owner)
{
    RECT rect = {};
    GetWindowRect(owner, &rect);
    m_layer.reset(CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, c_windowClassName, L"", WS_POPUP | WS_DISABLED,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, owner, nullptr, instance, nullptr));
    if (!m_layer) {
        return;
    }
    SetLayeredWindowAttributes(m_layer.get(), 0, c_alpha, LWA_ALPHA);
    styleWindowFrame(m_layer.get());
    ShowWindow(m_layer.get(), SW_SHOWNOACTIVATE);
}

Dimmer::~Dimmer() = default;

WNDCLASSEXW Dimmer::windowClass(HINSTANCE instance)
{
    WNDCLASSEXW windowClass = {sizeof(windowClass)};
    windowClass.lpfnWndProc = dimmerProc;
    windowClass.hInstance = instance;
    windowClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = c_windowClassName;
    return windowClass;
}

namespace
{

LRESULT CALLBACK dimmerProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_NCHITTEST: return HTTRANSPARENT;
        case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

}  // namespace
}  // namespace Varjo
