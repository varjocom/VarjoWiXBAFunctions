// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <string>

// Helpers for the windows and controls of the setup. Lengths are pixels at 96 DPI unless a function
// says otherwise.
namespace Varjo
{

// Returns the DPI of the window, or the DPI of the system on Windows versions before Windows 10
// version 1607, which have no DPI per window.
UINT windowDpi(HWND window);

// Scales a length in pixels at 96 DPI to dpi.
int scale(int pixels, UINT dpi);
RECT scaleRect(const RECT& rect, UINT dpi);

// Returns the text of the window, or an empty text for a null window.
std::wstring windowText(HWND window);

HFONT windowFont(HWND window);

// Returns the rectangle of the window in the client coordinates of its parent.
RECT windowRectInParent(HWND window);

// Checks the style rather than IsWindowVisible, which is false for every control while the window
// itself is still hidden.
bool hasVisibleStyle(HWND window);

// Asks Windows to send WM_MOUSELEAVE to the window when the mouse leaves it.
void trackMouseLeave(HWND window);

// Clicks the button of the window as the user would, through WM_COMMAND to the window. Does nothing
// for a null button.
void clickButton(HWND window, HWND button);

// Asks Windows 11 to round the corners of a top-level window and to draw no border around it.
// Earlier versions of Windows keep square corners.
void styleWindowFrame(HWND window);

// Hides the focus in a top-level window when the last input was from the mouse, as a dialog box
// does. The dialog manager shows it again when the user moves the focus with the keyboard.
void hideFocusUntilKeyboard(HWND window);

// A device context of a window from GetDC or GetWindowDC, which it releases when it goes away.
class WindowDc
{
public:
    // Client is the client area of the window, and Window the whole window with its frame.
    enum class Area { Client, Window };

    WindowDc(HWND window, Area area);
    ~WindowDc();
    WindowDc(const WindowDc&) = delete;
    WindowDc& operator=(const WindowDc&) = delete;

    HDC get() const;

private:
    HWND m_window;
    HDC m_dc;
};

// A registered window class, which is unregistered when the object goes away. Windows keeps the
// classes of a DLL registered after the DLL unloads, and WixStdBA can load the DLL again.
class WindowClass
{
public:
    explicit WindowClass(const WNDCLASSEXW& windowClass);
    ~WindowClass();
    WindowClass(const WindowClass&) = delete;
    WindowClass& operator=(const WindowClass&) = delete;

private:
    std::wstring m_name;
    HINSTANCE m_instance;
    bool m_registered;
};

}  // namespace Varjo
