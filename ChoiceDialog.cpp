// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ChoiceDialog.hpp"

#include "ControlState.hpp"
#include "Controls.hpp"
#include "Dimmer.hpp"
#include "Drawing.hpp"
#include "Handles.hpp"
#include "Subclass.hpp"
#include "Theme.hpp"
#include "WindowUtil.hpp"

#include <memory>
#include <optional>
#include <string>

namespace Varjo
{
namespace
{

constexpr wchar_t c_windowClassName[] = L"VarjoChoiceDialog";
constexpr DWORD c_style = WS_POPUP | WS_CLIPCHILDREN;
constexpr DWORD c_exStyle = WS_EX_CONTROLPARENT | WS_EX_TOOLWINDOW;

// The buttons have images of their own, which are opaque on the background of the dialog.
constexpr Theme::ButtonStyle c_leftStyle = {L"dialog-button-text", nullptr, Theme::c_textLabels};
constexpr Theme::ButtonStyle c_rightStyle = {L"dialog-button-finish", nullptr, Theme::c_outlinedLabels};

constexpr int c_leftId = 101;
constexpr int c_rightId = 102;

constexpr int c_width = 400;
constexpr int c_height = 144;
// The message starts one pixel lower than the box of its first line. GDI has no line height, and the
// offset puts the baseline where a line height of 20 pixels would.
constexpr RECT c_message = {24, 25, 376, 64};
constexpr RECT c_leftButton = {208, 88, 288, 120};
constexpr RECT c_rightButton = {296, 88, 376, 120};
constexpr int c_fontHeight = 14;
constexpr COLORREF c_messageColor = RGB(0xff, 0xff, 0xff);

struct DialogState {
    ImageCache& images;
    std::wstring message;
    DialogChoices choices;
    UINT dpi;
    UniqueFont font;
    UniqueFont buttonFont;
    HWND hovered = nullptr;
    bool done = false;
    DialogAnswer answer = DialogAnswer::Dismissed;
};

// Shows the hover state of a button of the dialog.
class ChoiceButton final : public Subclass
{
public:
    ChoiceButton(HWND button, DialogState& state);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    DialogState& m_state;
};

HFONT createFont(UINT dpi, const wchar_t* face);
HWND createDialog(HINSTANCE instance, HWND owner, DialogState& state);
// Runs the messages of the thread until the dialog has an answer. Returns the exit code of a
// WM_QUIT that ended the loop.
std::optional<int> runModalLoop(HWND dialog, const DialogState& state);
LRESULT CALLBACK dialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam);
std::optional<LRESULT> handleCommand(HWND dialog, DialogState& state, WORD id);
void createButton(HWND dialog, const RECT& layout, int id, const wchar_t* text, DialogState& state);
void paintDialog(HWND dialog, HDC dc, const DialogState& state);
void paintButton(const DRAWITEMSTRUCT& item, const DialogState& state);
void finish(HWND dialog, DialogState& state, DialogAnswer answer);
DialogAnswer answerOf(int id);

}  // namespace

DialogAnswer askInDialog(HINSTANCE instance, ImageCache& images, HWND owner, const wchar_t* message, const DialogChoices& choices)
{
    DialogState state{images, message, choices, windowDpi(owner)};
    state.font.reset(createFont(state.dpi, L"Segoe UI"));
    state.buttonFont.reset(createFont(state.dpi, L"Segoe UI Semibold"));

    const Dimmer dimmer(instance, owner);
    HWND dialog = createDialog(instance, owner, state);
    if (!dialog) {
        return DialogAnswer::Dismissed;
    }

    styleWindowFrame(dialog);
    hideFocusUntilKeyboard(dialog);
    EnableWindow(owner, FALSE);
    ShowWindow(dialog, SW_SHOW);
    SetFocus(GetDlgItem(dialog, choices.focusRight ? c_rightId : c_leftId));

    const std::optional<int> quitCode = runModalLoop(dialog, state);

    // The owner is enabled before the dialog is destroyed, so that Windows activates the owner.
    EnableWindow(owner, TRUE);
    DestroyWindow(dialog);
    if (quitCode) {
        PostQuitMessage(*quitCode);
    }
    return state.answer;
}

WNDCLASSEXW choiceDialogWindowClass(HINSTANCE instance)
{
    WNDCLASSEXW windowClass = {sizeof(windowClass)};
    windowClass.lpfnWndProc = dialogProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = c_windowClassName;
    return windowClass;
}

namespace
{

ChoiceButton::ChoiceButton(HWND button, DialogState& state)
    : Subclass(button, Scope::Window)
    , m_state(state)
{
}

std::optional<LRESULT> ChoiceButton::handleMessage(UINT message, WPARAM, LPARAM)
{
    if (message == WM_MOUSEMOVE && m_state.hovered != window()) {
        trackMouseLeave(window());
        m_state.hovered = window();
        InvalidateRect(window(), nullptr, FALSE);
    } else if (message == WM_MOUSELEAVE) {
        if (m_state.hovered == window()) {
            m_state.hovered = nullptr;
        }
        InvalidateRect(window(), nullptr, FALSE);
    }
    return std::nullopt;
}

HFONT createFont(UINT dpi, const wchar_t* face)
{
    return CreateFontW(-scale(c_fontHeight, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, face);
}

// Creates the dialog in the middle of owner.
HWND createDialog(HINSTANCE instance, HWND owner, DialogState& state)
{
    RECT frame = {0, 0, scale(c_width, state.dpi), scale(c_height, state.dpi)};
    AdjustWindowRectEx(&frame, c_style, FALSE, c_exStyle);
    const int width = frame.right - frame.left;
    const int height = frame.bottom - frame.top;
    RECT ownerRect = {};
    GetWindowRect(owner, &ownerRect);
    const int x = (ownerRect.left + ownerRect.right - width) / 2;
    const int y = (ownerRect.top + ownerRect.bottom - height) / 2;
    return CreateWindowExW(c_exStyle, c_windowClassName, L"", c_style, x, y, width, height, owner, nullptr, instance, &state);
}

std::optional<int> runModalLoop(HWND dialog, const DialogState& state)
{
    MSG msg = {};
    while (!state.done) {
        const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
        if (result == 0) {
            return static_cast<int>(msg.wParam);
        }
        if (result == -1) {
            break;
        }
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return std::nullopt;
}

LRESULT CALLBACK dialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    switch (message) {
        case WM_NCCREATE:
            state = static_cast<DialogState*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            break;
        case WM_CREATE:
            createButton(dialog, c_leftButton, c_leftId, state->choices.left, *state);
            createButton(dialog, c_rightButton, c_rightId, state->choices.right, *state);
            return 0;
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint = {};
            HDC dc = BeginPaint(dialog, &paint);
            paintDialog(dialog, dc, *state);
            EndPaint(dialog, &paint);
            return 0;
        }
        case WM_DRAWITEM: paintButton(*reinterpret_cast<DRAWITEMSTRUCT*>(lParam), *state); return TRUE;
        // The dialog has no caption, so the user moves it by any point outside the buttons.
        case WM_NCHITTEST: return HTCAPTION;
        case WM_COMMAND:
            if (const std::optional<LRESULT> result = handleCommand(dialog, *state, LOWORD(wParam))) {
                return *result;
            }
            break;
        case WM_CLOSE: finish(dialog, *state, DialogAnswer::Dismissed); return 0;
    }
    return DefWindowProcW(dialog, message, wParam, lParam);
}

std::optional<LRESULT> handleCommand(HWND dialog, DialogState& state, WORD id)
{
    switch (id) {
        case c_leftId:
        case c_rightId: finish(dialog, state, answerOf(id)); return 0;
        // Enter clicks the button that has the focus.
        case IDOK: finish(dialog, state, answerOf(GetDlgCtrlID(GetFocus()))); return 0;
        case IDCANCEL: finish(dialog, state, DialogAnswer::Dismissed); return 0;
    }
    return std::nullopt;
}

void createButton(HWND dialog, const RECT& layout, int id, const wchar_t* text, DialogState& state)
{
    const RECT rect = scaleRect(layout, state.dpi);
    HWND button = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, rect.left, rect.top, rect.right - rect.left,
        rect.bottom - rect.top, dialog, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), nullptr, nullptr);
    Subclass::install(std::make_unique<ChoiceButton>(button, state));
}

void paintDialog(HWND dialog, HDC dc, const DialogState& state)
{
    RECT client = {};
    GetClientRect(dialog, &client);
    fillRect(dc, client, Theme::c_dialogBackgroundColor);

    RECT message = scaleRect(c_message, state.dpi);
    const SelectedObject selectedFont(dc, state.font.get());
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c_messageColor);
    DrawTextW(dc, state.message.c_str(), -1, &message, DT_WORDBREAK | DT_NOPREFIX);
}

void paintButton(const DRAWITEMSTRUCT& item, const DialogState& state)
{
    const Theme::ButtonStyle& style = item.CtlID == c_leftId ? c_leftStyle : c_rightStyle;
    const bool focused = (item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT);
    const ButtonState stateOfButton = buttonState(true, item.itemState & ODS_SELECTED, item.hwndItem == state.hovered, focused);
    const std::wstring text = windowText(item.hwndItem);
    Controls::paintButtonImage(state.images, item.hDC, item.rcItem, stateOfButton, style);
    Controls::paintButtonLabel(state.images, item.hDC, item.rcItem, stateOfButton, style, text, state.buttonFont.get(), false, state.dpi);
}

void finish(HWND dialog, DialogState& state, DialogAnswer answer)
{
    state.answer = answer;
    state.done = true;
    ShowWindow(dialog, SW_HIDE);
}

DialogAnswer answerOf(int id) { return id == c_leftId ? DialogAnswer::Left : id == c_rightId ? DialogAnswer::Right : DialogAnswer::Dismissed; }

}  // namespace
}  // namespace Varjo
