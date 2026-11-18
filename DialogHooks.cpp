// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "DialogHooks.hpp"

#include "ChoiceDialog.hpp"
#include "ErrorPage.hpp"
#include "RunOutcome.hpp"
#include "Subclass.hpp"
#include "WindowUtil.hpp"

#include <dutil.h>

#include <IBootstrapperEngine.h>
#include <balutil.h>

#include <array>
#include <optional>

namespace Varjo
{
namespace
{

// MessageBoxW shows its text in the static control with this id.
constexpr int c_messageBoxTextId = 0xffff;

}  // namespace

// The message box of the cancel prompt ends with the answer of the user before Windows shows it,
// because EndDialog during WM_INITDIALOG keeps a dialog hidden.
class DialogHooks::CancelPrompt final : public Subclass
{
public:
    CancelPrompt(HWND dialog, DialogHooks& hooks);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    DialogHooks& m_hooks;
};

DialogHooks* DialogHooks::s_instance = nullptr;

DialogHooks::DialogHooks(HINSTANCE instance, ImageCache& images, const ErrorPage& errorPage, RunOutcome& outcome)
    : m_instance(instance)
    , m_images(images)
    , m_errorPage(errorPage)
    , m_outcome(outcome)
{
    s_instance = this;
}

DialogHooks::~DialogHooks()
{
    // The hooks go before the instance, so that no hook runs without it.
    m_engineThreadHook.reset();
    m_cancelPromptHook.reset();
    m_engineDimmer.reset();
    s_instance = nullptr;
}

void DialogHooks::setMainWindow(HWND window, BOOTSTRAPPER_DISPLAY display)
{
    m_mainWindow = window;
    m_display = display;
    m_caption = windowText(window);
}

void DialogHooks::startEngineThreadHook() { m_engineThreadHook.reset(SetWindowsHookExW(WH_CBT, engineThreadHook, nullptr, GetCurrentThreadId())); }

void DialogHooks::stopEngineThreadHook()
{
    m_engineThreadHook.reset();
    m_engineDimmer.reset();
}

bool DialogHooks::takeSuppressedMessageBox() { return m_messageBoxSuppressed.exchange(false); }

void DialogHooks::dropUnclaimedMessageBox()
{
    if (m_messageBoxSuppressed.exchange(false)) {
        BalLog(BOOTSTRAPPER_LOG_LEVEL_ERROR, "VARJO: A message box of WixStdBA did not show, and no event showed its message on the error page.");
    }
}

void DialogHooks::beginClose(bool withoutPrompt)
{
    m_closingWithoutPrompt = withoutPrompt;
    m_cancelPromptHook.reset(SetWindowsHookExW(WH_CBT, cancelPromptHook, nullptr, GetCurrentThreadId()));
}

void DialogHooks::endClose()
{
    m_cancelPromptHook.reset();
    m_closingWithoutPrompt = false;
}

// WixStdBA asks with MessageBoxW while it handles WM_CLOSE, so the hook catches the message box that
// the window creates during WM_CLOSE.
LRESULT CALLBACK DialogHooks::cancelPromptHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_CREATEWND) {
        const CREATESTRUCTW& create = *reinterpret_cast<CBT_CREATEWNDW*>(lParam)->lpcs;
        if (create.lpszClass == WC_DIALOG && create.hwndParent == s_instance->m_mainWindow) {
            Subclass::install(std::make_unique<CancelPrompt>(reinterpret_cast<HWND>(wParam), *s_instance));
            s_instance->m_cancelPromptHook.reset();
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

// WixStdBA shows its message boxes and the files-in-use dialog on the thread of the engine events,
// as dialogs that the window owns. The hook keeps a message box from being created while the error
// page can show its message instead, which makes MessageBoxW return 0. It dims the window under any
// other dialog.
LRESULT CALLBACK DialogHooks::engineThreadHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (s_instance->handleEngineThreadWindow(code, wParam, lParam)) {
        return 1;
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}

bool DialogHooks::isOwnedDialog(const CREATESTRUCTW& create) const
{
    return create.lpszClass == WC_DIALOG && m_mainWindow && create.hwndParent == m_mainWindow && !(create.style & WS_CHILD);
}

// WixStdBA gives its message boxes the caption of the theme as their title.
bool DialogHooks::isMessageBox(const CREATESTRUCTW& create) const
{
    return create.lpszName && !IS_INTRESOURCE(create.lpszName) && !m_caption.empty() && m_caption == create.lpszName;
}

bool DialogHooks::handleEngineThreadWindow(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HCBT_CREATEWND) {
        const CREATESTRUCTW& create = *reinterpret_cast<CBT_CREATEWNDW*>(lParam)->lpcs;
        if (!isOwnedDialog(create)) {
            return false;
        }
        if (isMessageBox(create) && m_errorPage.isAvailable(m_display)) {
            m_messageBoxSuppressed = true;
            return true;
        }
        if (!m_engineDimmer) {
            m_dimmedDialog = reinterpret_cast<HWND>(wParam);
            m_engineDimmer = std::make_unique<Dimmer>(m_instance, m_mainWindow);
        }
    } else if (code == HCBT_DESTROYWND && reinterpret_cast<HWND>(wParam) == m_dimmedDialog) {
        m_engineDimmer.reset();
        m_dimmedDialog = nullptr;
    }
    return false;
}

bool DialogHooks::confirmCancel(HWND owner, const wchar_t* message)
{
    const bool confirmed = m_closingWithoutPrompt || askInDialog(m_instance, m_images, owner, message, {L"Yes", L"No", false}) == DialogAnswer::Left;
    if (confirmed) {
        m_outcome.confirmCancel();
    }
    return confirmed;
}

DialogHooks::CancelPrompt::CancelPrompt(HWND dialog, DialogHooks& hooks)
    : Subclass(dialog, Scope::Window)
    , m_hooks(hooks)
{
}

std::optional<LRESULT> DialogHooks::CancelPrompt::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_INITDIALOG) {
        return std::nullopt;
    }
    // Destroying the main window during the choice dialog destroys this subclass too, so nothing
    // after confirmCancel may use its members.
    const HWND dialog = window();
    callNext(message, wParam, lParam);
    std::array<wchar_t, 512> text = {};
    GetDlgItemTextW(dialog, c_messageBoxTextId, text.data(), static_cast<int>(text.size()));
    const bool confirmed = m_hooks.confirmCancel(GetWindow(dialog, GW_OWNER), text.data());
    EndDialog(dialog, confirmed ? IDYES : IDNO);
    return FALSE;
}

}  // namespace Varjo
