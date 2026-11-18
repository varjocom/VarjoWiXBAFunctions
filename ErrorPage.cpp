// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ErrorPage.hpp"

#include "WindowUtil.hpp"

#include <dutil.h>

#include <IBootstrapperEngine.h>
#include <balutil.h>

namespace Varjo
{
namespace
{

// The theme shows the error page by these variables. VarjoErrorSeverity holds an ErrorSeverity, and
// VarjoErrorCode the code of the error. VarjoErrorButton1 to VarjoErrorButton3 hold the answers of
// the buttons from the left, as IDRETRY, or 0 for a button that the page hides.
constexpr wchar_t c_severityVariable[] = L"VarjoErrorSeverity";
constexpr wchar_t c_codeVariable[] = L"VarjoErrorCode";
constexpr std::array<const wchar_t*, c_maxPromptButtons> c_buttonVariables = {L"VarjoErrorButton1", L"VarjoErrorButton2", L"VarjoErrorButton3"};

}  // namespace

ErrorPage::ErrorPage()
    : m_showMessage(RegisterWindowMessageW(L"VarjoBAFunctionsShowErrorPage"))
    , m_answered(CreateEventW(nullptr, TRUE, FALSE, nullptr))
{
    for (auto& style : m_buttonStyles) {
        style = &Theme::c_textButton;
    }
}

const std::atomic<const Theme::ButtonStyle*>& ErrorPage::buttonStyle(size_t slot) const { return m_buttonStyles[slot]; }

void ErrorPage::setControls(const Controls& controls) { m_controls = controls; }

bool ErrorPage::isAvailable(BOOTSTRAPPER_DISPLAY display) const
{
    return display == BOOTSTRAPPER_DISPLAY_FULL && m_controls.showErrorPageButton && m_controls.showProgressPageButton && m_controls.messageText &&
           m_controls.progressBar && IsWindowVisible(m_controls.progressBar);
}

int ErrorPage::ask(ErrorSeverity severity, DWORD code, const std::wstring& text, DWORD uiHint)
{
    const PromptButtons buttons = promptButtons(uiHint);
    m_prompt = {};
    m_prompt.text = text;
    m_prompt.defaultSlot = promptButtonSlot(buttons, buttons.defaultIndex);
    m_prompt.closeAnswer = closeAnswer(buttons);
    for (size_t i = 0; i < buttons.count; ++i) {
        m_prompt.slotAnswers[promptButtonSlot(buttons, i)] = buttons.answers[i];
    }

    BalSetNumericVariable(c_severityVariable, static_cast<LONGLONG>(severity));
    BalSetNumericVariable(c_codeVariable, code);
    for (size_t slot = 0; slot < c_maxPromptButtons; ++slot) {
        BalSetNumericVariable(c_buttonVariables[slot], m_prompt.slotAnswers[slot]);
        m_buttonStyles[slot] = slot == m_prompt.defaultSlot ? &Theme::c_finishButton : &Theme::c_textButton;
    }

    ResetEvent(m_answered.get());
    m_promptWaiting = true;
    if (!PostMessageW(m_controls.window, m_showMessage, 0, 0)) {
        m_promptWaiting = false;
        return m_prompt.closeAnswer;
    }
    while (WaitForSingleObject(m_answered.get(), 1000) == WAIT_TIMEOUT) {
        // The window can go away before the UI thread has shown the page.
        if (!IsWindow(m_controls.window) && m_promptWaiting.exchange(false)) {
            m_prompt.answer = m_prompt.closeAnswer;
            break;
        }
    }
    BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "VARJO: The error page showed error %u, and the user answered %d.", code, m_prompt.answer);
    return m_prompt.answer;
}

bool ErrorPage::handleMessage(UINT message)
{
    if (message != m_showMessage) {
        return false;
    }
    show();
    return true;
}

bool ErrorPage::isShown() const { return m_shown; }

bool ErrorPage::answerWithButton(HWND control)
{
    for (size_t slot = 0; slot < c_maxPromptButtons; ++slot) {
        if (control && control == m_controls.buttons[slot]) {
            // The engine thread may write the next prompt as soon as it no longer waits, so the
            // answer is read only after this thread has taken the wait.
            if (m_promptWaiting.exchange(false)) {
                finishAnswer(m_prompt.slotAnswers[slot]);
            }
            return true;
        }
    }
    return false;
}

void ErrorPage::answerClose()
{
    if (m_promptWaiting.exchange(false)) {
        finishAnswer(m_prompt.closeAnswer);
    }
}

void ErrorPage::releaseOnDestroy()
{
    m_shown = false;
    if (m_promptWaiting.exchange(false)) {
        m_prompt.answer = m_prompt.closeAnswer;
        SetEvent(m_answered.get());
    }
}

void ErrorPage::show()
{
    if (!m_promptWaiting) {
        return;
    }
    m_shown = true;
    // thmutil shows the texts of the Progress page from the theme again when the page returns.
    m_savedActionText = windowText(m_controls.actionText);
    clickButton(m_controls.window, m_controls.showErrorPageButton);
    SetWindowTextW(m_controls.messageText, m_prompt.text.c_str());
    for (HWND button : m_controls.buttons) {
        if (button) {
            InvalidateRect(button, nullptr, FALSE);
        }
    }
    if (HWND button = m_controls.buttons[m_prompt.defaultSlot]) {
        SendMessageW(m_controls.window, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(button), TRUE);
    }
}

// Returns to the Progress page before the engine thread goes on, so that the page of WixStdBA shows
// when WixStdBA changes the page.
void ErrorPage::finishAnswer(int answer)
{
    m_prompt.answer = answer;
    if (m_shown) {
        m_shown = false;
        clickButton(m_controls.window, m_controls.showProgressPageButton);
        if (m_controls.actionText && !m_savedActionText.empty()) {
            SetWindowTextW(m_controls.actionText, m_savedActionText.c_str());
        }
    }
    SetEvent(m_answered.get());
}

}  // namespace Varjo
