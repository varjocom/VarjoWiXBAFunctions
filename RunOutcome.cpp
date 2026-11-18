// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "RunOutcome.hpp"

#include "ErrorPrompt.hpp"

#include <dutil.h>

#include <IBootstrapperEngine.h>
#include <balutil.h>

namespace Varjo
{
namespace
{

// The theme shows the texts of a canceled setup while this variable is 1.
constexpr wchar_t c_canceledVariable[] = L"VarjoSetupCanceled";

// The Failure page shows the texts of the error that ended the run by this code.
constexpr wchar_t c_failureCodeVariable[] = L"VarjoFailureCode";

// The number of the error of Windows Installer that reports too little disk space.
constexpr DWORD c_outOfDiskSpaceError = 1601;

}  // namespace

RunOutcome::RunOutcome()
    : m_setCanceledMessage(RegisterWindowMessageW(L"VarjoBAFunctionsSetCanceled"))
{
}

void RunOutcome::setMainWindow(HWND window) { m_window = window; }

void RunOutcome::noteAnswer(DWORD installerError, const std::wstring& text, int answer)
{
    if (m_stoppedAtError) {
        return;
    }
    const bool continues = continuesPastMessage(answer);
    {
        const std::lock_guard<std::mutex> lock(m_failureMutex);
        m_failureText = continues ? std::wstring() : text;
    }
    BalSetNumericVariable(c_failureCodeVariable, continues ? 0 : installerError);
    // The Failure page of the disk space error has its own text and no message box.
    m_failureMessageHidden = m_canceled || (!continues && installerError == c_outOfDiskSpaceError);
    if (answer == IDCANCEL || answer == IDABORT) {
        m_stoppedAtError = true;
    }
}

void RunOutcome::markCanceled()
{
    m_canceled = true;
    m_failureMessageHidden = true;
    BalSetNumericVariable(c_canceledVariable, 1);
}

void RunOutcome::confirmCancel()
{
    m_canceled = true;
    m_failureMessageHidden = true;
    PostMessageW(m_window, m_setCanceledMessage, 0, 0);
}

bool RunOutcome::handleMessage(UINT message)
{
    if (message != m_setCanceledMessage) {
        return false;
    }
    BalSetNumericVariable(c_canceledVariable, 1);
    return true;
}

bool RunOutcome::stoppedAtError() const { return m_stoppedAtError; }

std::wstring RunOutcome::failureText() const
{
    const std::lock_guard<std::mutex> lock(m_failureMutex);
    return m_failureText;
}

const std::atomic<bool>& RunOutcome::canceled() const { return m_canceled; }

const std::atomic<bool>& RunOutcome::failureMessageHidden() const { return m_failureMessageHidden; }

}  // namespace Varjo
