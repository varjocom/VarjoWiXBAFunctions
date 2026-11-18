// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <windows.h>

namespace Varjo
{

void setRichEditTextColor(HWND richEdit, COLORREF color);

// Asks for a printer and prints the content of the rich edit control in black. The control shows
// its text in screenColor again afterwards. documentName names the print job and may be empty.
void printRichEdit(HWND owner, HWND richEdit, const wchar_t* documentName, COLORREF screenColor);

}  // namespace Varjo
