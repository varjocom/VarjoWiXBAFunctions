// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Subclass.hpp"

#include <commctrl.h>

namespace Varjo
{

Subclass::~Subclass() = default;

bool Subclass::install(std::unique_ptr<Subclass> subclass)
{
    // SetWindowSubclass tells the subclasses of one window apart by the procedure and the id. The
    // object is the id of its subclasses, and the window owns it once they are in place.
    const auto id = reinterpret_cast<UINT_PTR>(subclass.get());
    const auto data = reinterpret_cast<DWORD_PTR>(subclass.get());
    if (!SetWindowSubclass(subclass->m_window, windowProc, id, data)) {
        return false;
    }
    if (subclass->m_scope == Scope::WindowAndParent && !SetWindowSubclass(subclass->m_parent, parentProc, id, data)) {
        RemoveWindowSubclass(subclass->m_window, windowProc, id);
        return false;
    }
    subclass.release();
    return true;
}

Subclass::Subclass(HWND window, Scope scope)
    : m_window(window)
    , m_parent(GetParent(window))
    , m_scope(scope)
{
}

HWND Subclass::window() const { return m_window; }

HWND Subclass::parent() const { return m_parent; }

std::optional<LRESULT> Subclass::handleMessage(UINT, WPARAM, LPARAM) { return std::nullopt; }

std::optional<LRESULT> Subclass::handleParentMessage(UINT, WPARAM, LPARAM) { return std::nullopt; }

void Subclass::onDestroy() {}

LRESULT Subclass::callNext(UINT message, WPARAM wParam, LPARAM lParam) const { return DefSubclassProc(m_window, message, wParam, lParam); }

LRESULT Subclass::callNextOfParent(UINT message, WPARAM wParam, LPARAM lParam) const { return DefSubclassProc(m_parent, message, wParam, lParam); }

LRESULT CALLBACK Subclass::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
{
    auto* const self = reinterpret_cast<Subclass*>(data);
    if (message == WM_NCDESTROY) {
        self->onDestroy();
        // The children of a window are destroyed before the window itself, so the parent still has
        // its subclass.
        if (self->m_scope == Scope::WindowAndParent) {
            RemoveWindowSubclass(self->m_parent, parentProc, id);
        }
        RemoveWindowSubclass(window, windowProc, id);
        std::unique_ptr<Subclass>{self}.reset();
        return DefSubclassProc(window, message, wParam, lParam);
    }
    if (const std::optional<LRESULT> result = self->handleMessage(message, wParam, lParam)) {
        return *result;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

LRESULT CALLBACK Subclass::parentProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR data)
{
    auto* const self = reinterpret_cast<Subclass*>(data);
    if (const std::optional<LRESULT> result = self->handleParentMessage(message, wParam, lParam)) {
        return *result;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

}  // namespace Varjo
