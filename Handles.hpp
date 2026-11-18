// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <memory>
#include <type_traits>

// Owners of the Windows resources that BAFunctions creates. Each one frees its resource when it goes
// out of scope.
namespace Varjo
{

struct GdiObjectDeleter {
    void operator()(HGDIOBJ object) const { DeleteObject(object); }
};

struct DcDeleter {
    void operator()(HDC dc) const { DeleteDC(dc); }
};

struct HandleDeleter {
    void operator()(HANDLE handle) const { CloseHandle(handle); }
};

struct HookDeleter {
    void operator()(HHOOK hook) const { UnhookWindowsHookEx(hook); }
};

struct GlobalMemoryDeleter {
    void operator()(HGLOBAL memory) const { GlobalFree(memory); }
};

struct WindowDeleter {
    void operator()(HWND window) const { DestroyWindow(window); }
};

using UniqueBrush = std::unique_ptr<std::remove_pointer_t<HBRUSH>, GdiObjectDeleter>;
using UniqueFont = std::unique_ptr<std::remove_pointer_t<HFONT>, GdiObjectDeleter>;
using UniqueBitmap = std::unique_ptr<std::remove_pointer_t<HBITMAP>, GdiObjectDeleter>;
using UniqueRegion = std::unique_ptr<std::remove_pointer_t<HRGN>, GdiObjectDeleter>;
// A device context that CreateDC, CreateCompatibleDC or PrintDlgW created.
using UniqueDc = std::unique_ptr<std::remove_pointer_t<HDC>, DcDeleter>;
// A handle that CloseHandle closes, such as an event.
using UniqueHandle = std::unique_ptr<void, HandleDeleter>;
using UniqueHook = std::unique_ptr<std::remove_pointer_t<HHOOK>, HookDeleter>;
using UniqueGlobalMemory = std::unique_ptr<void, GlobalMemoryDeleter>;
using UniqueWindow = std::unique_ptr<std::remove_pointer_t<HWND>, WindowDeleter>;

}  // namespace Varjo
