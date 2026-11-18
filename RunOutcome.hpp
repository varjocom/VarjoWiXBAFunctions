// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <atomic>
#include <mutex>
#include <string>

namespace Varjo
{

// How the run ended, as the Failure page shows it: canceled, or failed at the error that the user
// saw last. The thread of the engine events reports the errors and the cancels of the packages, and
// the UI thread reports the cancel prompt.
class RunOutcome
{
public:
    RunOutcome();
    RunOutcome(const RunOutcome&) = delete;
    RunOutcome& operator=(const RunOutcome&) = delete;

    // The UI thread sets the window before the engine sends its events.
    void setMainWindow(HWND window);

    // Runs on the thread of the engine events after the user answered a message. The Failure page
    // shows the last message after which the run did not go on. After an answer that stopped the
    // run, the messages of the rollback do not replace it. installerError is the number of an error
    // of Windows Installer, or 0.
    void noteAnswer(DWORD installerError, const std::wstring& text, int answer);

    // Runs on the thread of the engine events when a package or the run ends as canceled.
    void markCanceled();

    // Runs on the UI thread when the user confirms the cancel prompt. WixStdBA holds the lock that
    // its progress callbacks wait for, so the variable of the theme is set after the prompt has
    // returned, by the message that handleMessage takes.
    void confirmCancel();

    // Takes the message that confirmCancel posts to the window. Returns whether it was that message.
    bool handleMessage(UINT message);

    bool stoppedAtError() const;
    std::wstring failureText() const;
    const std::atomic<bool>& canceled() const;
    const std::atomic<bool>& failureMessageHidden() const;

private:
    const UINT m_setCanceledMessage;
    HWND m_window = nullptr;
    std::atomic<bool> m_canceled{false};
    std::atomic<bool> m_failureMessageHidden{false};
    std::atomic<bool> m_stoppedAtError{false};
    mutable std::mutex m_failureMutex;
    std::wstring m_failureText;
};

}  // namespace Varjo
