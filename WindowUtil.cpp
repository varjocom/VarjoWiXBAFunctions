// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "WindowUtil.hpp"

#include <dwmapi.h>

namespace Varjo
{
namespace
{

// DWMWA_WINDOW_CORNER_PREFERENCE, DWMWCP_ROUND, DWMWA_BORDER_COLOR and DWMWA_COLOR_NONE of the
// Windows SDK 10.0.22000 and later, which the build may predate.
constexpr DWORD c_windowCornerPreference = 33;
constexpr DWORD c_cornerRound = 2;
constexpr DWORD c_borderColor = 34;
constexpr COLORREF c_colorNone = 0xfffffffe;

}  // namespace

UINT windowDpi(HWND window)
{
    // The setup runs on Windows 10 versions that have no GetDpiForWindow, so the DLL must not import
    // it.
    using GetDpiForWindowFunction = UINT(WINAPI*)(HWND);
    static const auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFunction>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (getDpiForWindow) {
        return getDpiForWindow(window);
    }
    const WindowDc dc(window, WindowDc::Area::Client);
    return static_cast<UINT>(GetDeviceCaps(dc.get(), LOGPIXELSY));
}

int scale(int pixels, UINT dpi) { return MulDiv(pixels, static_cast<int>(dpi), 96); }

RECT scaleRect(const RECT& rect, UINT dpi) { return {scale(rect.left, dpi), scale(rect.top, dpi), scale(rect.right, dpi), scale(rect.bottom, dpi)}; }

std::wstring windowText(HWND window)
{
    if (!window) {
        return {};
    }
    const int length = GetWindowTextLengthW(window);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(window, text.data(), length + 1);
    text.resize(static_cast<size_t>(length));
    return text;
}

HFONT windowFont(HWND window) { return reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0)); }

RECT windowRectInParent(HWND window)
{
    RECT rect = {};
    GetWindowRect(window, &rect);
    MapWindowPoints(nullptr, GetParent(window), reinterpret_cast<POINT*>(&rect), 2);
    return rect;
}

bool hasVisibleStyle(HWND window) { return GetWindowLongPtrW(window, GWL_STYLE) & WS_VISIBLE; }

void trackMouseLeave(HWND window)
{
    TRACKMOUSEEVENT track = {sizeof(track), TME_LEAVE, window, 0};
    TrackMouseEvent(&track);
}

void clickButton(HWND window, HWND button)
{
    if (button) {
        SendMessageW(window, WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(button), BN_CLICKED), reinterpret_cast<LPARAM>(button));
    }
}

void styleWindowFrame(HWND window)
{
    DwmSetWindowAttribute(window, c_windowCornerPreference, &c_cornerRound, sizeof(c_cornerRound));
    DwmSetWindowAttribute(window, c_borderColor, &c_colorNone, sizeof(c_colorNone));
}

void hideFocusUntilKeyboard(HWND window) { SendMessageW(window, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0); }

WindowDc::WindowDc(HWND window, Area area)
    : m_window(window)
    , m_dc(area == Area::Client ? GetDC(window) : GetWindowDC(window))
{
}

WindowDc::~WindowDc() { ReleaseDC(m_window, m_dc); }

HDC WindowDc::get() const { return m_dc; }

WindowClass::WindowClass(const WNDCLASSEXW& windowClass)
    : m_name(windowClass.lpszClassName)
    , m_instance(windowClass.hInstance)
    , m_registered(RegisterClassExW(&windowClass) != 0)
{
}

WindowClass::~WindowClass()
{
    if (m_registered) {
        UnregisterClassW(m_name.c_str(), m_instance);
    }
}

}  // namespace Varjo
