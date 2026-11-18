// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "DialogHooks.hpp"
#include "ErrorPage.hpp"
#include "ErrorPrompt.hpp"
#include "RunOutcome.hpp"

#include <windows.h>

#include <msiquery.h>

// The WiX headers expect the includer to provide their dependencies, as the WiX sample does.
#include <dutil.h>
#include <dictutil.h>
#include <strutil.h>

#include <BalBaseBAFunctions.h>
#include <balinfo.h>

#include <array>
#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Varjo
{

class ImageCache;

// The BAFunctions of the Varjo setup. WixStdBA, the standard bootstrapper application of WiX,
// forwards its events to this object after it has handled them itself, from the thread of the
// engine events and from the UI thread. The object implements what the theme of the setup cannot
// express. The image cache must outlive the object.
class VarjoBAFunctions final : public CBalBaseBAFunctions
{
public:
    VarjoBAFunctions(HMODULE module, ImageCache& images);
    ~VarjoBAFunctions() override;
    VarjoBAFunctions(const VarjoBAFunctions&) = delete;
    VarjoBAFunctions& operator=(const VarjoBAFunctions&) = delete;

    STDMETHODIMP OnCreate(IBootstrapperEngine* engine, BOOTSTRAPPER_COMMAND* command) override;
    STDMETHODIMP OnStartup() override;
    STDMETHODIMP OnShutdown(BOOTSTRAPPER_SHUTDOWN_ACTION* action) override;
    STDMETHODIMP OnDetectComplete(HRESULT status, BOOL eligibleForCleanup) override;
    STDMETHODIMP OnElevateComplete(HRESULT status) override;
    STDMETHODIMP OnError(BOOTSTRAPPER_ERROR_TYPE type, LPCWSTR packageId, DWORD code, LPCWSTR error, DWORD uiHint, DWORD dataCount, LPCWSTR* data,
        int recommendation, int* result) override;
    STDMETHODIMP OnExecuteMsiMessage(
        LPCWSTR packageId, INSTALLMESSAGE messageType, DWORD uiHint, LPCWSTR message, DWORD dataCount, LPCWSTR* data, int recommendation, int* result) override;
    STDMETHODIMP OnExecutePackageComplete(LPCWSTR packageId, HRESULT status, BOOTSTRAPPER_APPLY_RESTART restart,
        BOOTSTRAPPER_EXECUTEPACKAGECOMPLETE_ACTION recommendation, BOOTSTRAPPER_EXECUTEPACKAGECOMPLETE_ACTION* action) override;
    STDMETHODIMP OnExecuteComplete(HRESULT status) override;
    STDMETHODIMP OnThemeLoaded(HWND window) override;
    STDMETHODIMP OnThemeControlLoaded(LPCWSTR name, WORD id, HWND control, BOOL* processed) override;
    STDMETHODIMP OnThemeControlWmCommand(WPARAM wParam, LPCWSTR name, WORD id, HWND control, BOOL* processed, LRESULT* result) override;

    // Runs on the thread of the engine events after the object has handled an event of the engine.
    void finishEngineEvent();

private:
    class MainWindow;
    class ActionText;
    class FailureMessage;
    class RestartButton;

    HWND* controlSlot(std::wstring_view name);
    void styleControl(std::wstring_view name, HWND control);
    bool styleImageButton(std::wstring_view name, HWND control);
    void fitMessageLabels();
    void styleRichEdit();

    std::optional<LRESULT> handleMainWindowMessage(MainWindow& window, UINT message, WPARAM wParam, LPARAM lParam);
    std::optional<LRESULT> handleRegisteredMessage(UINT message, LPARAM lParam);
    std::optional<LRESULT> onNextDialogControl(MainWindow& window, WPARAM wParam, LPARAM lParam);
    LRESULT onClose(MainWindow& window, WPARAM wParam, LPARAM lParam);
    std::optional<LRESULT> onCommand(MainWindow& window, WPARAM wParam, LPARAM lParam);
    std::optional<LRESULT> onNotify(LPARAM lParam);
    void onMainWindowDestroyed();

    bool isInstallPageFocus(HWND target) const;
    void askRestart(HWND restartButton);
    void openLog();
    void bringToFront();
    bool isLinkControl(HWND control) const;
    bool isLogLink(HWND control) const;

    ImageCache& m_images;
    BOOTSTRAPPER_DISPLAY m_display = BOOTSTRAPPER_DISPLAY_UNKNOWN;
    BOOTSTRAPPER_ACTION m_action = BOOTSTRAPPER_ACTION_UNKNOWN;
    BAL_INFO_COMMAND m_command = {};

    const UINT m_bringToFrontMessage;
    const UINT m_showHelpPageMessage;
    const UINT m_askRestartMessage;

    RunOutcome m_outcome;
    ErrorPage m_errorPage;
    DialogHooks m_dialogHooks;

    // The UI thread uses these.
    bool m_closingWithoutPrompt = false;
    bool m_restartAsked = false;
    // Atomic for keepHidden, which also takes the flags that the thread of the engine events sets.
    std::atomic<bool> m_restartDeclined{false};

    // The thread of the engine events writes these, and the UI thread reads them.
    std::atomic<bool> m_showShield{false};
    std::atomic<bool> m_settingActionText{false};

    // The controls are created on the UI thread before the engine sends the events that use them.
    HWND m_installButton = nullptr;
    HWND m_installFolder = nullptr;
    HWND m_checkbox = nullptr;
    HWND m_licenseConsentText = nullptr;
    HWND m_showLicenseButton = nullptr;
    HWND m_licenseDocumentName = nullptr;
    HWND m_showErrorPageButton = nullptr;
    HWND m_showProgressPageButton = nullptr;
    HWND m_showHelpPageButton = nullptr;
    HWND m_acceptAndInstallButton = nullptr;
    HWND m_printButton = nullptr;
    HWND m_removeCancelButton = nullptr;
    HWND m_progressBar = nullptr;
    HWND m_actionBarDivider = nullptr;
    HWND m_errorMessageText = nullptr;
    HWND m_errorLogFileLink = nullptr;
    std::array<HWND, c_maxPromptButtons> m_errorButtons = {};
    HWND m_failureMessageText = nullptr;
    HWND m_failureLogFileLink = nullptr;
    HWND m_successRestartButton = nullptr;
    HWND m_failureRestartButton = nullptr;
    HWND m_successRestartText = nullptr;
    HWND m_failureRestartText = nullptr;
    HWND m_successCloseButton = nullptr;
    HWND m_failureCloseButton = nullptr;
    HWND m_restartLaterLabel = nullptr;
    HWND m_actionText = nullptr;
    HWND m_richEdit = nullptr;
    std::vector<HWND> m_linkControls;
};

}  // namespace Varjo
