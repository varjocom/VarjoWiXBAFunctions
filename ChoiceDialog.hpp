// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

namespace Varjo
{

class ImageCache;

// The two buttons of a choice dialog: a text button on the left and an outlined button on the
// right. focusRight gives the right button the focus when the dialog opens. Otherwise the left
// button takes it.
struct DialogChoices {
    const wchar_t* left;
    const wchar_t* right;
    bool focusRight;
};

// Dismissed is the answer to Escape and to closing the dialog.
enum class DialogAnswer { Left, Right, Dismissed };

// Asks with a modal dialog in the colors of the theme, and dims the window of owner under it. The
// dialog disables owner while it is open, and it runs on the thread of owner. The window classes of
// choiceDialogWindowClass and Dimmer::windowClass must be registered.
DialogAnswer askInDialog(HINSTANCE instance, ImageCache& images, HWND owner, const wchar_t* message, const DialogChoices& choices);

WNDCLASSEXW choiceDialogWindowClass(HINSTANCE instance);

}  // namespace Varjo
