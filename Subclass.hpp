// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <memory>
#include <optional>

namespace Varjo
{

// A subclass of a window that the window owns. The object sees the messages of the window, and with
// Scope::WindowAndParent also the messages of the parent of the window, before the procedures that
// the window had before. It goes away when the window is destroyed. Everything runs on the thread of
// the window.
class Subclass
{
public:
    virtual ~Subclass();
    Subclass(const Subclass&) = delete;
    Subclass& operator=(const Subclass&) = delete;

    // Installs the subclass on its window, which then owns it. Of the subclasses of one window, the
    // one installed last sees a message first. Returns false and destroys the subclass when Windows
    // refuses it.
    static bool install(std::unique_ptr<Subclass> subclass);

protected:
    enum class Scope { Window, WindowAndParent };

    Subclass(HWND window, Scope scope);

    HWND window() const;
    HWND parent() const;

    // Returns the result of a message that the subclass handled, or nothing for a message that goes
    // on to the earlier procedures of the window.
    virtual std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    virtual std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam);
    // Runs when the window is destroyed, before the subclass goes away.
    virtual void onDestroy();

    // Pass a message on to the earlier procedures of the window or of the parent. Only a handler of
    // the same window may call these.
    LRESULT callNext(UINT message, WPARAM wParam, LPARAM lParam) const;
    LRESULT callNextOfParent(UINT message, WPARAM wParam, LPARAM lParam) const;

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
    static LRESULT CALLBACK parentProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);

    const HWND m_window;
    const HWND m_parent;
    const Scope m_scope;
};

}  // namespace Varjo
