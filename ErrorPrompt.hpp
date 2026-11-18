// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

#include <array>
#include <cstddef>
#include <initializer_list>

// The buttons of the error page, which shows the errors and warnings that WixStdBA would show in a
// message box. Nothing here needs a window.
namespace Varjo
{

// The theme shows the heading and the icon of the error page by this value of VarjoErrorSeverity.
enum class ErrorSeverity { Error = 1, Warning = 2, PackageMessage = 3 };

constexpr size_t c_maxPromptButtons = 3;

// The buttons of a message box from left to right, as the answers that MessageBoxW returns, such as
// IDRETRY. defaultIndex is the index of the default button.
struct PromptButtons {
    std::array<int, c_maxPromptButtons> answers;
    size_t count;
    size_t defaultIndex;
};

// Returns the buttons that MessageBoxW shows for the type in uiHint, with its default button. A
// default button beyond the last button is the first button, as in MessageBoxW. An unknown type
// shows OK.
constexpr PromptButtons promptButtons(UINT uiHint)
{
    PromptButtons buttons = {{IDOK, 0, 0}, 1, 0};
    switch (uiHint & MB_TYPEMASK) {
        case MB_OKCANCEL: buttons = {{IDOK, IDCANCEL, 0}, 2, 0}; break;
        case MB_ABORTRETRYIGNORE: buttons = {{IDABORT, IDRETRY, IDIGNORE}, 3, 0}; break;
        case MB_YESNOCANCEL: buttons = {{IDYES, IDNO, IDCANCEL}, 3, 0}; break;
        case MB_YESNO: buttons = {{IDYES, IDNO, 0}, 2, 0}; break;
        case MB_RETRYCANCEL: buttons = {{IDRETRY, IDCANCEL, 0}, 2, 0}; break;
        case MB_CANCELTRYCONTINUE: buttons = {{IDCANCEL, IDTRYAGAIN, IDCONTINUE}, 3, 0}; break;
    }
    const size_t defaultIndex = (uiHint & MB_DEFMASK) >> 8;
    buttons.defaultIndex = defaultIndex < buttons.count ? defaultIndex : 0;
    return buttons;
}

constexpr bool hasAnswer(const PromptButtons& buttons, int answer)
{
    for (size_t i = 0; i < buttons.count; ++i) {
        if (buttons.answers[i] == answer) {
            return true;
        }
    }
    return false;
}

// Returns the answer to the message when the user closes the window instead of choosing a button.
// That is the button that stops: Cancel, then Abort, then No. A message with none of them has only
// OK.
constexpr int closeAnswer(const PromptButtons& buttons)
{
    for (const int answer : {IDCANCEL, IDABORT, IDNO}) {
        if (hasAnswer(buttons, answer)) {
            return answer;
        }
    }
    return buttons.answers[0];
}

// Returns the position in the action bar of the button at index, counted from the left of the three
// positions. The buttons are aligned to the right.
constexpr size_t promptButtonSlot(const PromptButtons& buttons, size_t index) { return c_maxPromptButtons - buttons.count + index; }

// Returns whether the run goes on past the message after this answer, as after Retry. The Failure
// page shows only a message after which the run did not go on.
constexpr bool continuesPastMessage(int answer) { return answer == IDRETRY || answer == IDIGNORE || answer == IDTRYAGAIN || answer == IDCONTINUE; }

}  // namespace Varjo
