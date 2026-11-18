// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "ErrorPrompt.hpp"
#include "Handles.hpp"
#include "Theme.hpp"

#include <windows.h>

#include <msiquery.h>

#include <BootstrapperApplicationTypes.h>

#include <array>
#include <atomic>
#include <string>

namespace Varjo
{

// The error page of the theme, which shows an error or a warning of the run in place of the
// message box of WixStdBA, with the buttons of the message box. The thread of the engine events asks
// and waits, and the UI thread shows the page and returns the answer of the user. The page takes
// the place of the Progress page while it shows.
class ErrorPage
{
public:
    // The controls of the theme that the page needs.
    struct Controls {
        // The main window and the hidden buttons that switch between the error page and the
        // Progress page.
        HWND window;
        HWND showErrorPageButton;
        HWND showProgressPageButton;
        HWND messageText;
        HWND progressBar;
        // The status line of the Progress page, whose text the page keeps.
        HWND actionText;
        // The buttons from the left.
        std::array<HWND, c_maxPromptButtons> buttons;
    };

    ErrorPage();
    ErrorPage(const ErrorPage&) = delete;
    ErrorPage& operator=(const ErrorPage&) = delete;

    // The style of the button at slot, which the default button changes. The UI thread draws the
    // buttons in these styles.
    const std::atomic<const Theme::ButtonStyle*>& buttonStyle(size_t slot) const;

    // The UI thread sets the controls once the theme has loaded, before the engine sends its events.
    void setControls(const Controls& controls);

    // Runs on the thread of the engine events. Returns whether the page can show now. It shows only
    // in the full UI while the Progress page shows, and checking the visibility does not wait for the
    // UI thread.
    bool isAvailable(BOOTSTRAPPER_DISPLAY display) const;

    // Runs on the thread of the engine events. Shows the message on the page and waits until the user
    // answers on the UI thread. Returns the answer, as MessageBoxW would.
    int ask(ErrorSeverity severity, DWORD code, const std::wstring& text, DWORD uiHint);

    // The UI thread passes the messages of the main window here. Returns whether the message was
    // the one with which ask shows the page.
    bool handleMessage(UINT message);

    // Run on the UI thread.
    bool isShown() const;
    // Answers with the button control, and returns whether control is a button of the page.
    bool answerWithButton(HWND control);
    // Answers as the user would by closing the window.
    void answerClose();
    // Releases the waiting thread when the window goes away. The controls are gone, so the page
    // stays.
    void releaseOnDestroy();

private:
    // The message that the page shows. The engine thread writes it before it posts
    // m_showMessage. The UI thread writes answer before it sets m_answered.
    struct Prompt {
        std::wstring text;
        std::array<int, c_maxPromptButtons> slotAnswers = {};
        size_t defaultSlot = 0;
        int closeAnswer = IDCANCEL;
        int answer = IDCANCEL;
    };

    void show();
    // Returns to the Progress page and releases the waiting thread with the answer. The caller has
    // taken m_promptWaiting.
    void finishAnswer(int answer);

    const UINT m_showMessage;
    Controls m_controls = {};

    Prompt m_prompt;
    // The engine thread waits for an answer while this is true.
    std::atomic<bool> m_promptWaiting{false};
    UniqueHandle m_answered;
    std::array<std::atomic<const Theme::ButtonStyle*>, c_maxPromptButtons> m_buttonStyles;

    // The UI thread uses these.
    bool m_shown = false;
    std::wstring m_savedActionText;
};

}  // namespace Varjo
