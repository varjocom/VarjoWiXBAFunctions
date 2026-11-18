// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "Dimmer.hpp"
#include "Handles.hpp"

#include <windows.h>

#include <msiquery.h>

#include <BootstrapperApplicationTypes.h>

#include <atomic>
#include <memory>
#include <string>

namespace Varjo
{

class ErrorPage;
class ImageCache;
class RunOutcome;

// Takes over the dialogs that WixStdBA shows over the main window. On the thread of the engine
// events, a message box gives way to the error page, and the window is dimmed under any other
// dialog, such as the files-in-use dialog. On the UI thread, the cancel prompt of WixStdBA becomes
// a choice dialog. Hooks have no context, so only one object may exist at a time.
class DialogHooks
{
public:
    DialogHooks(HINSTANCE instance, ImageCache& images, const ErrorPage& errorPage, RunOutcome& outcome);
    ~DialogHooks();
    DialogHooks(const DialogHooks&) = delete;
    DialogHooks& operator=(const DialogHooks&) = delete;

    // The UI thread sets the window once the theme has loaded, before the engine sends its events.
    // display is the display of the run.
    void setMainWindow(HWND window, BOOTSTRAPPER_DISPLAY display);

    // Run on the thread of the engine events, when the engine starts and when it shuts down.
    void startEngineThreadHook();
    void stopEngineThreadHook();

    // Runs on the thread of the engine events in the handler of an event. Returns whether the hook
    // kept a message box of WixStdBA from showing for this event, and takes it.
    bool takeSuppressedMessageBox();

    // Runs on the thread of the engine events after the DLL has handled an event. A message box
    // that the hook kept from WixStdBA belongs to the event that WixStdBA handled just before, so a
    // box that no handler took is not left for a later event.
    void dropUnclaimedMessageBox();

    // Run on the UI thread around the handling of WM_CLOSE by WixStdBA, which asks with MessageBoxW.
    // With withoutPrompt, the window closes as if the user had confirmed.
    void beginClose(bool withoutPrompt);
    void endClose();

private:
    class CancelPrompt;

    static LRESULT CALLBACK cancelPromptHook(int code, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK engineThreadHook(int code, WPARAM wParam, LPARAM lParam);

    bool isOwnedDialog(const CREATESTRUCTW& create) const;
    bool isMessageBox(const CREATESTRUCTW& create) const;
    // Returns whether the window that the engine thread creates must not be created.
    bool handleEngineThreadWindow(int code, WPARAM wParam, LPARAM lParam);
    bool confirmCancel(HWND owner, const wchar_t* message);

    static DialogHooks* s_instance;

    const HINSTANCE m_instance;
    ImageCache& m_images;
    const ErrorPage& m_errorPage;
    RunOutcome& m_outcome;

    // The UI thread writes these before the engine sends the events that read them.
    HWND m_mainWindow = nullptr;
    BOOTSTRAPPER_DISPLAY m_display = BOOTSTRAPPER_DISPLAY_UNKNOWN;
    std::wstring m_caption;

    // The UI thread uses these.
    UniqueHook m_cancelPromptHook;
    bool m_closingWithoutPrompt = false;

    // The thread of the engine events uses these.
    UniqueHook m_engineThreadHook;
    std::atomic<bool> m_messageBoxSuppressed{false};
    std::unique_ptr<Dimmer> m_engineDimmer;
    HWND m_dimmedDialog = nullptr;
};

}  // namespace Varjo
