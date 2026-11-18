// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "VarjoBAFunctions.hpp"

#include "ActionText.hpp"
#include "ChoiceDialog.hpp"
#include "Controls.hpp"
#include "LicensePrinting.hpp"
#include "ShutdownAction.hpp"
#include "Subclass.hpp"
#include "Theme.hpp"
#include "WindowUtil.hpp"

#include <commctrl.h>
#include <richedit.h>
#include <shellapi.h>

#include <balutil.h>

#include <cwchar>
#include <memory>

namespace Varjo
{
namespace
{

constexpr std::array<const wchar_t*, c_maxPromptButtons> c_errorButtonNames = {L"ErrorButton1", L"ErrorButton2", L"ErrorButton3"};

// The id of the link in LicenseConsentText that shows the License page.
constexpr wchar_t c_licenseLinkId[] = L"license";

// The gap between a message and the log link below it.
constexpr int c_logLinkGap = 16;
// The log link of the error page stays this far above the action bar, and the log link of the
// Failure page this far above the restart text.
constexpr int c_errorLogLinkMargin = 16;
constexpr int c_failureLogLinkMargin = 8;

// The image buttons of the theme, and how BAFunctions draws their labels.
struct ThemeButton {
    const wchar_t* name;
    const Theme::ButtonStyle* style;
};

constexpr std::array c_buttons{
    ThemeButton{L"BrowseButton", &Theme::c_browseButton},
    ThemeButton{L"PrintButton", &Theme::c_printButton},
    ThemeButton{L"LicenseBackButton", &Theme::c_textButton},
    ThemeButton{L"ProgressCancelButton", &Theme::c_textButton},
    ThemeButton{L"RemoveCancelButton", &Theme::c_textButton},
    ThemeButton{L"UninstallButton", &Theme::c_removeButton},
    ThemeButton{L"SuccessRestartButton", &Theme::c_textButton},
    ThemeButton{L"FailureRestartButton", &Theme::c_textButton},
    ThemeButton{L"SuccessCloseButton", &Theme::c_finishButton},
    ThemeButton{L"FailureCloseButton", &Theme::c_finishButton},
    ThemeButton{L"HelpCloseButton", &Theme::c_finishButton},
};

struct WixStringDeleter {
    void operator()(LPWSTR text) const { StrFree(text); }
};
using UniqueWixString = std::unique_ptr<WCHAR, WixStringDeleter>;

bool isShownAndEnabled(HWND control);
bool arrowKeyDown();
std::wstring errorText(BOOTSTRAPPER_ERROR_TYPE type, DWORD code, LPCWSTR error);
HWND findRichEdit(HWND window);
BOOL CALLBACK findRichEditChild(HWND child, LPARAM data);

}  // namespace

// The subclass of the main window, for the messages that need BAFunctions before WixStdBA and
// thmutil.
class VarjoBAFunctions::MainWindow final : public Subclass
{
public:
    MainWindow(HWND window, VarjoBAFunctions& owner);

    // Passes the message on to WixStdBA and thmutil.
    LRESULT passOn(UINT message, WPARAM wParam, LPARAM lParam) const;

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
    void onDestroy() override;

private:
    VarjoBAFunctions& m_owner;
};

// WixStdBA sets the whole message of each action into ExecuteProgressActionDataText from the thread
// of the engine events, before the DLL sees the event. The message starts with the time and the name
// of the action. The label keeps only the text that the DLL sets from that thread, and the text that
// thmutil and the DLL set on the UI thread.
class VarjoBAFunctions::ActionText final : public Subclass
{
public:
    ActionText(HWND label, const VarjoBAFunctions& owner);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    const VarjoBAFunctions& m_owner;
};

// WixStdBA writes the code and the system text of the result of a failed run into
// FailureMessageText. The label shows the message of the error that ended the run instead, when the
// user saw one.
class VarjoBAFunctions::FailureMessage final : public Subclass
{
public:
    FailureMessage(HWND label, const VarjoBAFunctions& owner);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    const VarjoBAFunctions& m_owner;
};

// WixStdBA shows the Restart button of the Success or the Failure page when the run needs a restart.
// The DLL then asks in a dialog over the page.
class VarjoBAFunctions::RestartButton final : public Subclass
{
public:
    RestartButton(HWND button, VarjoBAFunctions& owner);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    VarjoBAFunctions& m_owner;
};

VarjoBAFunctions::VarjoBAFunctions(HMODULE module, ImageCache& images)
    : CBalBaseBAFunctions(module)
    , m_images(images)
    , m_bringToFrontMessage(RegisterWindowMessageW(L"VarjoBAFunctionsBringToFront"))
    , m_showHelpPageMessage(RegisterWindowMessageW(L"VarjoBAFunctionsShowHelpPage"))
    , m_askRestartMessage(RegisterWindowMessageW(L"VarjoBAFunctionsAskRestart"))
    , m_dialogHooks(module, images, m_errorPage, m_outcome)
{
}

VarjoBAFunctions::~VarjoBAFunctions() { BalInfoUninitializeCommandLine(&m_command); }

STDMETHODIMP VarjoBAFunctions::OnCreate(IBootstrapperEngine* engine, BOOTSTRAPPER_COMMAND* command)
{
    HRESULT hr = CBalBaseBAFunctions::OnCreate(engine, command);
    if (SUCCEEDED(hr)) {
        m_display = command->display;
        m_action = command->action;
        hr = BalInfoParseCommandLine(&m_command, command);
    }
    return hr;
}

STDMETHODIMP VarjoBAFunctions::OnStartup()
{
    m_dialogHooks.startEngineThreadHook();
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnShutdown(BOOTSTRAPPER_SHUTDOWN_ACTION* action)
{
    m_dialogHooks.stopEngineThreadHook();

    const BOOTSTRAPPER_SHUTDOWN_ACTION result = shutdownAction(*action, m_display, m_command.restart == BAL_INFO_RESTART_ALWAYS);
    if (result != *action) {
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "VARJO: The machine needs a restart. A run without the full UI restarts it only with /forcerestart.");
        *action = result;
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnDetectComplete(HRESULT, BOOL)
{
    LONGLONG elevated = 0;
    BalGetNumericVariable(L"WixBundleElevated", &elevated);
    m_showShield = !elevated;

    // WixStdBA may have shown the Install page before this event reaches the DLL.
    for (HWND button : {m_installButton, m_acceptAndInstallButton}) {
        if (button) {
            InvalidateRect(button, nullptr, FALSE);
        }
    }

    // WixStdBA of WiX 7.0.0 and 7.0.1 shows the Help page and then detects, which leaves the window
    // without a page, as wixtoolset/issues#8808 reports. WixStdBA has posted the change to the
    // detected state already, so the Help page shows after it.
    if (m_action == BOOTSTRAPPER_ACTION_HELP && m_display == BOOTSTRAPPER_DISPLAY_FULL && m_hwndParent) {
        PostMessageW(m_hwndParent, m_showHelpPageMessage, 0, 0);
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnElevateComplete(HRESULT status)
{
    // The window of the setup drops behind other windows after the UAC prompt, as
    // wixtoolset/issues#9317 reports. The window belongs to the UI thread.
    if (SUCCEEDED(status) && m_hwndParent) {
        PostMessageW(m_hwndParent, m_bringToFrontMessage, 0, 0);
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnError(BOOTSTRAPPER_ERROR_TYPE type, LPCWSTR, DWORD code, LPCWSTR error, DWORD uiHint, DWORD, LPCWSTR*, int, int* result)
{
    const std::wstring text = errorText(type, code, error);
    // Only an error of Windows Installer has the number of the error of Windows Installer.
    const DWORD installerError = type == BOOTSTRAPPER_ERROR_TYPE_WINDOWS_INSTALLER ? code : 0;
    if (m_dialogHooks.takeSuppressedMessageBox()) {
        *result = m_errorPage.ask(ErrorSeverity::Error, installerError, text, uiHint);
    }
    m_outcome.noteAnswer(installerError, text, *result);
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnExecuteMsiMessage(
    LPCWSTR, INSTALLMESSAGE messageType, DWORD uiHint, LPCWSTR message, DWORD dataCount, LPCWSTR* data, int, int* result)
{
    if ((messageType == INSTALLMESSAGE_WARNING || messageType == INSTALLMESSAGE_USER) && m_dialogHooks.takeSuppressedMessageBox()) {
        const ErrorSeverity severity = messageType == INSTALLMESSAGE_WARNING ? ErrorSeverity::Warning : ErrorSeverity::PackageMessage;
        const std::wstring text = message ? message : L"";
        *result = m_errorPage.ask(severity, 0, text, uiHint);
        // The run goes on after a warning unless the answer stops it.
        if (*result == IDCANCEL || *result == IDABORT) {
            m_outcome.noteAnswer(0, text, *result);
        }
    }

    // The setup shows only the description of the action. An action without a description leaves
    // the text as it is.
    if (messageType == INSTALLMESSAGE_ACTIONSTART && m_actionText) {
        const std::wstring description = dataCount >= 2 && data[1] ? actionDescription(data[1]) : std::wstring();
        if (!description.empty()) {
            m_settingActionText = true;
            SetWindowTextW(m_actionText, description.c_str());
            m_settingActionText = false;
        }
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnExecutePackageComplete(
    LPCWSTR, HRESULT status, BOOTSTRAPPER_APPLY_RESTART, BOOTSTRAPPER_EXECUTEPACKAGECOMPLETE_ACTION, BOOTSTRAPPER_EXECUTEPACKAGECOMPLETE_ACTION*)
{
    // A package that the user canceled in one of its own dialogs ends here. WixStdBA does not forward
    // this event after a cancel through its cancel prompt, which RunOutcome::confirmCancel marks. A
    // package that the user stopped at an error ends with the same code, and the run then failed
    // rather than being canceled.
    if (status == HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT) && !m_outcome.stoppedAtError()) {
        m_outcome.markCanceled();
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnExecuteComplete(HRESULT status)
{
    // WixStdBA cancels an EXE package that runs without its cancel prompt, and it does not forward
    // OnExecutePackageComplete of that package to BAFunctions.
    if (status == HRESULT_FROM_WIN32(ERROR_INSTALL_USEREXIT) && !m_outcome.stoppedAtError()) {
        m_outcome.markCanceled();
    }
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnThemeLoaded(HWND window)
{
    const HRESULT hr = CBalBaseBAFunctions::OnThemeLoaded(window);
    m_outcome.setMainWindow(window);
    m_dialogHooks.setMainWindow(window, m_display);
    m_errorPage.setControls({window, m_showErrorPageButton, m_showProgressPageButton, m_errorMessageText, m_progressBar, m_actionText, m_errorButtons});

    styleWindowFrame(window);
    hideFocusUntilKeyboard(window);
    Subclass::install(std::make_unique<MainWindow>(window, *this));
    fitMessageLabels();
    styleRichEdit();
    return hr;
}

STDMETHODIMP VarjoBAFunctions::OnThemeControlLoaded(LPCWSTR name, WORD, HWND control, BOOL*)
{
    const std::wstring_view controlName = name ? name : L"";
    if (HWND* slot = controlSlot(controlName)) {
        *slot = control;
    }
    for (size_t i = 0; i < c_maxPromptButtons; ++i) {
        if (controlName == c_errorButtonNames[i]) {
            m_errorButtons[i] = control;
            Controls::drawButtonInChosenStyle(m_images, control, m_errorPage.buttonStyle(i));
        }
    }
    styleControl(controlName, control);
    return S_OK;
}

STDMETHODIMP VarjoBAFunctions::OnThemeControlWmCommand(WPARAM wParam, LPCWSTR name, WORD, HWND, BOOL* processed, LRESULT* result)
{
    // thmutil has no action that minimizes the window.
    if (HIWORD(wParam) == BN_CLICKED && name && std::wstring_view(name) == L"MinimizeButton") {
        ShowWindow(m_hwndParent, SW_MINIMIZE);
        *processed = TRUE;
        *result = 0;
    }
    return S_OK;
}

void VarjoBAFunctions::finishEngineEvent() { m_dialogHooks.dropUnclaimedMessageBox(); }

// Returns the member that keeps the control of the theme with the name, or null.
HWND* VarjoBAFunctions::controlSlot(std::wstring_view name)
{
    struct Slot {
        std::wstring_view name;
        HWND* control;
    };
    const std::array slots{
        Slot{L"InstallButton", &m_installButton},
        Slot{L"InstallFolder", &m_installFolder},
        Slot{L"EulaAcceptCheckbox", &m_checkbox},
        Slot{L"LicenseConsentText", &m_licenseConsentText},
        Slot{L"ShowLicenseButton", &m_showLicenseButton},
        Slot{L"LicenseDocumentName", &m_licenseDocumentName},
        Slot{L"ShowErrorPageButton", &m_showErrorPageButton},
        Slot{L"ShowProgressPageButton", &m_showProgressPageButton},
        Slot{L"ShowHelpPageButton", &m_showHelpPageButton},
        Slot{L"RestartLaterLabel", &m_restartLaterLabel},
        Slot{L"AcceptAndInstallButton", &m_acceptAndInstallButton},
        Slot{L"PrintButton", &m_printButton},
        Slot{L"RemoveCancelButton", &m_removeCancelButton},
        Slot{L"OverallCalculatedProgressbar", &m_progressBar},
        Slot{L"ActionBarDivider", &m_actionBarDivider},
        Slot{L"ExecuteProgressActionDataText", &m_actionText},
        Slot{L"ErrorMessageText", &m_errorMessageText},
        Slot{L"ErrorLogFileLink", &m_errorLogFileLink},
        Slot{L"FailureMessageText", &m_failureMessageText},
        Slot{L"FailureLogFileLink", &m_failureLogFileLink},
        Slot{L"SuccessRestartButton", &m_successRestartButton},
        Slot{L"FailureRestartButton", &m_failureRestartButton},
        Slot{L"SuccessRestartRequiredText", &m_successRestartText},
        Slot{L"FailureRestartRequiredText", &m_failureRestartText},
        Slot{L"SuccessCloseButton", &m_successCloseButton},
        Slot{L"FailureCloseButton", &m_failureCloseButton},
    };
    for (const Slot& slot : slots) {
        if (slot.name == name) {
            return slot.control;
        }
    }
    return nullptr;
}

void VarjoBAFunctions::styleControl(std::wstring_view name, HWND control)
{
    if (styleImageButton(name, control)) {
        if (name == L"SuccessRestartButton" || name == L"FailureRestartButton") {
            Subclass::install(std::make_unique<RestartButton>(control, *this));
            Controls::keepHidden(control, m_restartDeclined);
        }
    } else if (name == L"MinimizeButton" || name == L"CloseButton") {
        Controls::endHoverOnLeave(control);
    } else if (name == L"InstallButton" || name == L"AcceptAndInstallButton") {
        Controls::drawButtonWithOptionalIcon(m_images, control, Theme::c_acceptInstallButton, m_showShield);
    } else if (name == L"InstallFolder") {
        Controls::drawEditField(control);
    } else if (name == L"EulaAcceptCheckbox") {
        Controls::drawCheckbox(m_images, control);
    } else if (name == L"LicenseConsentText" || name == L"PrivacyPolicyText" || name == L"ErrorLogFileLink") {
        // SysLink colors its links in the color of the system. A SysLink control with the style
        // LWS_USECUSTOMTEXT, which the theme sets, lets its parent change the color.
        m_linkControls.push_back(control);
    } else if (name == L"FailureLogFileLink") {
        m_linkControls.push_back(control);
        Controls::keepHidden(control, m_outcome.canceled());
    } else if (name == L"ShowLicenseButton" || name == L"ShowErrorPageButton" || name == L"ShowProgressPageButton" || name == L"ShowHelpPageButton" ||
               name == L"RestartLaterLabel" || name == L"LicenseDocumentName") {
        Controls::keepHidden(control);
    } else if (name == L"OverallCalculatedProgressbar") {
        Controls::drawProgressBar(control);
    } else if (name == L"ExecuteProgressActionDataText") {
        Subclass::install(std::make_unique<ActionText>(control, *this));
    } else if (name == L"FailureMessageText") {
        Controls::keepHidden(control, m_outcome.failureMessageHidden());
    }
}

// Draws the image buttons of the theme that have a style of their own. Returns whether the control
// is one of them.
bool VarjoBAFunctions::styleImageButton(std::wstring_view name, HWND control)
{
    for (const ThemeButton& button : c_buttons) {
        if (name == button.name) {
            Controls::drawButton(m_images, control, *button.style);
            return true;
        }
    }
    return false;
}

// Runs when the whole theme has loaded, because each label needs the controls below it.
void VarjoBAFunctions::fitMessageLabels()
{
    if (m_errorMessageText && m_errorLogFileLink && m_actionBarDivider) {
        Controls::fitLabel(m_errorMessageText, {m_errorLogFileLink, c_logLinkGap, false, m_actionBarDivider, c_errorLogLinkMargin});
    }
    if (m_failureMessageText && m_failureLogFileLink && m_failureRestartText) {
        Controls::fitLabel(m_failureMessageText, {m_failureLogFileLink, c_logLinkGap, true, m_failureRestartText, c_failureLogLinkMargin});
        // The text that WixStdBA writes passes this subclass before the one that fits the label to it.
        Subclass::install(std::make_unique<FailureMessage>(m_failureMessageText, *this));
    }
}

void VarjoBAFunctions::styleRichEdit()
{
    m_richEdit = findRichEdit(m_hwndParent);
    if (m_richEdit) {
        Controls::drawLicenseBox(m_richEdit);
    }
}

std::optional<LRESULT> VarjoBAFunctions::handleMainWindowMessage(MainWindow& window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (const std::optional<LRESULT> result = handleRegisteredMessage(message, lParam)) {
        return result;
    }
    switch (message) {
        case WM_NEXTDLGCTL: return onNextDialogControl(window, wParam, lParam);
        case WM_CLOSE: return onClose(window, wParam, lParam);
        case WM_COMMAND: return onCommand(window, wParam, lParam);
        case WM_NOTIFY: return onNotify(lParam);
    }
    return std::nullopt;
}

// Handles the messages that the DLL posts to the window from the thread of the engine events.
std::optional<LRESULT> VarjoBAFunctions::handleRegisteredMessage(UINT message, LPARAM lParam)
{
    if (m_outcome.handleMessage(message) || m_errorPage.handleMessage(message)) {
        return 0;
    }
    if (message == m_bringToFrontMessage) {
        bringToFront();
        return 0;
    }
    if (message == m_showHelpPageMessage) {
        clickButton(m_hwndParent, m_showHelpPageButton);
        return 0;
    }
    if (message == m_askRestartMessage) {
        askRestart(reinterpret_cast<HWND>(lParam));
        return 0;
    }
    return std::nullopt;
}

// The Install page would open with the folder focused and selected. thmutil gives the page's focus
// to its first tab stop, which is the folder. WixStdBA then gives it to the install button, and while
// that button is disabled, to the next enabled tab stop after it. The check box takes the focus in
// both cases. The dialog manager sends the same message when an arrow key moves the focus.
std::optional<LRESULT> VarjoBAFunctions::onNextDialogControl(MainWindow& window, WPARAM wParam, LPARAM lParam)
{
    if (LOWORD(lParam) && isInstallPageFocus(reinterpret_cast<HWND>(wParam)) && !arrowKeyDown() && m_checkbox && isShownAndEnabled(m_checkbox)) {
        return window.passOn(WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(m_checkbox), lParam);
    }
    return std::nullopt;
}

LRESULT VarjoBAFunctions::onClose(MainWindow& window, WPARAM wParam, LPARAM lParam)
{
    if (m_errorPage.isShown()) {
        m_errorPage.answerClose();
        return 0;
    }
    // The window of /? shows only the Help page, so closing it loses nothing. The window loses the
    // focus to the cancel prompt and does not get it back when the user answers No.
    const HWND focus = GetFocus();
    m_dialogHooks.beginClose(m_closingWithoutPrompt || m_action == BOOTSTRAPPER_ACTION_HELP);
    const LRESULT result = window.passOn(WM_CLOSE, wParam, lParam);
    m_dialogHooks.endClose();
    m_closingWithoutPrompt = false;
    if (focus && IsWindow(focus) && IsChild(m_hwndParent, focus) && IsWindowVisible(focus) && IsWindowEnabled(focus)) {
        SetFocus(focus);
    }
    return result;
}

std::optional<LRESULT> VarjoBAFunctions::onCommand(MainWindow& window, WPARAM wParam, LPARAM lParam)
{
    // Escape sends IDCANCEL from no control.
    if (m_errorPage.isShown() && LOWORD(wParam) == IDCANCEL && !lParam) {
        m_errorPage.answerClose();
        return 0;
    }
    if (HIWORD(wParam) != BN_CLICKED) {
        return std::nullopt;
    }
    const HWND control = reinterpret_cast<HWND>(lParam);
    if (m_errorPage.answerWithButton(control)) {
        return 0;
    }
    if (control == m_acceptAndInstallButton) {
        // The button returns to the Install page first, because WixStdBA replaces only its own pages
        // with the Progress page.
        const LRESULT result = window.passOn(WM_COMMAND, wParam, lParam);
        SendMessageW(m_checkbox, BM_SETCHECK, BST_CHECKED, 0);
        clickButton(m_hwndParent, m_checkbox);
        clickButton(m_hwndParent, m_installButton);
        return result;
    }
    if (control == m_removeCancelButton) {
        // The button closes the window, and the user has already chosen not to remove.
        m_closingWithoutPrompt = true;
        return std::nullopt;
    }
    if (control == m_printButton) {
        if (m_richEdit) {
            printRichEdit(m_hwndParent, m_richEdit, windowText(m_licenseDocumentName).c_str(), Theme::c_textColor);
        }
        return 0;
    }
    return std::nullopt;
}

std::optional<LRESULT> VarjoBAFunctions::onNotify(LPARAM lParam)
{
    const NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
    if (header->code == NM_CUSTOMTEXT && isLinkControl(header->hwndFrom)) {
        // The link font of SysLink is underlined. The log file links keep the underline in the color
        // of the text, and the other links of the theme have no underline.
        const NMCUSTOMTEXT* text = reinterpret_cast<NMCUSTOMTEXT*>(lParam);
        if (text->fLink && isLogLink(header->hwndFrom)) {
            SetTextColor(text->hDC, Theme::c_textColor);
        } else if (text->fLink) {
            SetTextColor(text->hDC, Theme::c_linkColor);
            SelectObject(text->hDC, windowFont(header->hwndFrom));
        }
        return 0;
    }
    const bool activated = header->code == NM_CLICK || header->code == NM_RETURN;
    if (activated && header->hwndFrom == m_licenseConsentText && std::wstring_view(reinterpret_cast<const NMLINK*>(lParam)->item.szID) == c_licenseLinkId) {
        clickButton(m_hwndParent, m_showLicenseButton);
        return 1;
    }
    if (activated && header->hwndFrom == m_errorLogFileLink) {
        openLog();
        return 1;
    }
    return std::nullopt;
}

void VarjoBAFunctions::onMainWindowDestroyed() { m_errorPage.releaseOnDestroy(); }

bool VarjoBAFunctions::isInstallPageFocus(HWND target) const
{
    if (target == m_installFolder) {
        return true;
    }
    return m_installButton && !IsWindowEnabled(m_installButton) && target == GetNextDlgTabItem(m_hwndParent, m_installButton, FALSE);
}

// Runs on the UI thread.
void VarjoBAFunctions::askRestart(HWND restartButton)
{
    const bool success = restartButton == m_successRestartButton;
    const std::wstring message = windowText(success ? m_successRestartText : m_failureRestartText);
    const std::wstring later = windowText(m_restartLaterLabel);
    const std::wstring restart = windowText(restartButton);
    if (message.empty() || later.empty() || restart.empty()) {
        return;
    }
    if (askInDialog(m_hModule, m_images, m_hwndParent, message.c_str(), {later.c_str(), restart.c_str(), true}) == DialogAnswer::Right) {
        clickButton(m_hwndParent, restartButton);
        return;
    }
    m_restartDeclined = true;
    ShowWindow(restartButton, SW_HIDE);
    if (HWND finish = success ? m_successCloseButton : m_failureCloseButton) {
        SendMessageW(m_hwndParent, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(finish), TRUE);
    }
}

void VarjoBAFunctions::openLog()
{
    LPWSTR log = nullptr;
    const HRESULT hr = BalGetStringVariable(L"WixBundleLog", &log);
    const UniqueWixString ownedLog(log);
    if (SUCCEEDED(hr) && log && *log) {
        ShellExecuteW(m_hwndParent, L"open", log, nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void VarjoBAFunctions::bringToFront()
{
    HWND foreground = GetForegroundWindow();
    if (foreground == m_hwndParent) {
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "VARJO: The window of the setup is in front after the elevation.");
        return;
    }
    if (SetForegroundWindow(m_hwndParent)) {
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "VARJO: Brought the window of the setup to the front after the elevation.");
        return;
    }

    // Windows lets a thread that shares the input state of the foreground thread take the
    // foreground.
    const DWORD foregroundThread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD thisThread = GetCurrentThreadId();
    const bool attached = foregroundThread && foregroundThread != thisThread && AttachThreadInput(thisThread, foregroundThread, TRUE);
    BringWindowToTop(m_hwndParent);
    const bool inFront = SetForegroundWindow(m_hwndParent);
    if (attached) {
        AttachThreadInput(thisThread, foregroundThread, FALSE);
    }
    BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD,
        inFront ? "VARJO: Brought the window of the setup to the front after the elevation, with the input state of the foreground thread."
                : "VARJO: Windows did not let the setup bring its window to the front after the elevation.");
}

bool VarjoBAFunctions::isLinkControl(HWND control) const
{
    for (HWND link : m_linkControls) {
        if (link == control) {
            return true;
        }
    }
    return false;
}

bool VarjoBAFunctions::isLogLink(HWND control) const { return control == m_failureLogFileLink || control == m_errorLogFileLink; }

VarjoBAFunctions::MainWindow::MainWindow(HWND window, VarjoBAFunctions& owner)
    : Subclass(window, Scope::Window)
    , m_owner(owner)
{
}

LRESULT VarjoBAFunctions::MainWindow::passOn(UINT message, WPARAM wParam, LPARAM lParam) const { return callNext(message, wParam, lParam); }

std::optional<LRESULT> VarjoBAFunctions::MainWindow::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    return m_owner.handleMainWindowMessage(*this, message, wParam, lParam);
}

void VarjoBAFunctions::MainWindow::onDestroy() { m_owner.onMainWindowDestroyed(); }

VarjoBAFunctions::ActionText::ActionText(HWND label, const VarjoBAFunctions& owner)
    : Subclass(label, Scope::Window)
    , m_owner(owner)
{
}

std::optional<LRESULT> VarjoBAFunctions::ActionText::handleMessage(UINT message, WPARAM, LPARAM)
{
    if (message == WM_SETTEXT && InSendMessage() && !m_owner.m_settingActionText) {
        return TRUE;
    }
    return std::nullopt;
}

VarjoBAFunctions::FailureMessage::FailureMessage(HWND label, const VarjoBAFunctions& owner)
    : Subclass(label, Scope::Window)
    , m_owner(owner)
{
}

std::optional<LRESULT> VarjoBAFunctions::FailureMessage::handleMessage(UINT message, WPARAM wParam, LPARAM)
{
    if (message != WM_SETTEXT) {
        return std::nullopt;
    }
    const std::wstring failure = m_owner.m_outcome.failureText();
    if (failure.empty()) {
        return std::nullopt;
    }
    return callNext(message, wParam, reinterpret_cast<LPARAM>(failure.c_str()));
}

VarjoBAFunctions::RestartButton::RestartButton(HWND button, VarjoBAFunctions& owner)
    : Subclass(button, Scope::Window)
    , m_owner(owner)
{
}

std::optional<LRESULT> VarjoBAFunctions::RestartButton::handleMessage(UINT message, WPARAM, LPARAM lParam)
{
    const bool shown = message == WM_WINDOWPOSCHANGED && (reinterpret_cast<const WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW);
    if (shown && !m_owner.m_restartAsked && m_owner.m_display == BOOTSTRAPPER_DISPLAY_FULL) {
        m_owner.m_restartAsked = true;
        PostMessageW(m_owner.m_hwndParent, m_owner.m_askRestartMessage, 0, reinterpret_cast<LPARAM>(window()));
    }
    return std::nullopt;
}

namespace
{

// Checks the style rather than IsWindowVisible, which is false for every control while the window
// itself is still hidden.
bool isShownAndEnabled(HWND control) { return hasVisibleStyle(control) && IsWindowEnabled(control); }

bool arrowKeyDown()
{
    for (const int key : {VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN}) {
        if (GetKeyState(key) & 0x8000) {
            return true;
        }
    }
    return false;
}

// Returns the text that WixStdBA would show for the error.
std::wstring errorText(BOOTSTRAPPER_ERROR_TYPE type, DWORD code, LPCWSTR error)
{
    if (error && *error && type == BOOTSTRAPPER_ERROR_TYPE_WINDOWS_INSTALLER) {
        return error;
    }
    LPWSTR text = nullptr;
    const HRESULT hr = StrAllocFromError(&text, code, nullptr);
    const UniqueWixString ownedText(text);
    if (SUCCEEDED(hr) && text && *text) {
        return text;
    }
    std::array<wchar_t, 16> hex = {};
    swprintf_s(hex.data(), hex.size(), L"0x%x", code);
    return hex.data();
}

// WixStdBA loads the license into EulaRichedit and does not report the control to BAFunctions, so
// the control is found by its window class.
HWND findRichEdit(HWND window)
{
    HWND richEdit = nullptr;
    EnumChildWindows(window, findRichEditChild, reinterpret_cast<LPARAM>(&richEdit));
    return richEdit;
}

BOOL CALLBACK findRichEditChild(HWND child, LPARAM data)
{
    std::array<wchar_t, 64> className = {};
    GetClassNameW(child, className.data(), static_cast<int>(className.size()));
    const std::wstring_view name = className.data();
    if (name == MSFTEDIT_CLASS || name == RICHEDIT_CLASSW) {
        *reinterpret_cast<HWND*>(data) = child;
        return FALSE;
    }
    return TRUE;
}

}  // namespace
}  // namespace Varjo
