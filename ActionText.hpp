// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <cwctype>
#include <string>
#include <string_view>

namespace Varjo
{

// Returns the description of a Windows Installer action from the second field of its ACTIONSTART
// message. The field holds the description and then the template of the action data, as in
// "Copying new files File: [1], Directory: [9], Size: [6]". The template starts with a label. The
// label ends at the first ": [", and it starts at the nearest word before it that starts with a
// capital letter.
inline std::wstring actionDescription(std::wstring_view text)
{
    const size_t colon = text.find(L": [");
    if (colon == std::wstring_view::npos) {
        return std::wstring(text);
    }

    size_t labelStart = colon;
    for (;;) {
        const size_t space = labelStart == 0 ? std::wstring_view::npos : text.rfind(L' ', labelStart - 1);
        labelStart = space == std::wstring_view::npos ? 0 : space + 1;
        if (labelStart == 0 || std::iswupper(text[labelStart])) {
            break;
        }
        labelStart = space;
    }

    const size_t end = labelStart == 0 ? std::wstring_view::npos : text.find_last_not_of(L' ', labelStart - 1);
    return end == std::wstring_view::npos ? std::wstring() : std::wstring(text.substr(0, end + 1));
}

}  // namespace Varjo
