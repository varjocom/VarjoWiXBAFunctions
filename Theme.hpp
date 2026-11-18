// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "ControlState.hpp"

#include <windows.h>

#include <array>

// The colors and the button styles of the theme of the setup, which BAFunctions needs where it draws
// itself. The theme writes its colors in BGR order.
namespace Varjo
{
namespace Theme
{

// c_backgroundColor is the background of the window, c_textColor the color of its text and
// c_fieldBackgroundColor the background of the edit box.
constexpr COLORREF c_backgroundColor = RGB(0x1e, 0x1e, 0x1e);
constexpr COLORREF c_textColor = RGB(0xce, 0xce, 0xce);
constexpr COLORREF c_fieldBackgroundColor = RGB(0x3d, 0x3d, 0x3d);

constexpr COLORREF c_linkColor = RGB(0x00, 0xd9, 0xb9);
constexpr COLORREF c_licenseBackgroundColor = RGB(0x0f, 0x0f, 0x0f);
constexpr COLORREF c_dialogBackgroundColor = RGB(0x27, 0x27, 0x27);

// The label color of a button in each ButtonState.
using LabelColors = std::array<COLORREF, c_buttonStateCount>;

constexpr COLORREF c_lightLabelColor = RGB(0xff, 0xff, 0xff);
constexpr COLORREF c_darkLabelColor = RGB(0x0f, 0x0f, 0x0f);
constexpr COLORREF c_removeLabelColor = RGB(0xff, 0x5e, 0x57);
constexpr COLORREF c_disabledLabelColor = RGB(0x74, 0x74, 0x74);

// The outlined buttons have a light background under the mouse, so their label turns dark.
constexpr std::array c_outlinedLabels{c_lightLabelColor, c_darkLabelColor, c_darkLabelColor, c_lightLabelColor, c_disabledLabelColor};
static_assert(c_outlinedLabels.size() == c_buttonStateCount);
constexpr std::array c_removeLabels{c_removeLabelColor, c_lightLabelColor, c_lightLabelColor, c_removeLabelColor, c_disabledLabelColor};
static_assert(c_removeLabels.size() == c_buttonStateCount);
constexpr std::array c_printLabels{c_textColor, c_lightLabelColor, c_lightLabelColor, c_textColor, c_disabledLabelColor};
static_assert(c_printLabels.size() == c_buttonStateCount);
constexpr std::array c_textLabels{c_lightLabelColor, c_lightLabelColor, c_lightLabelColor, c_lightLabelColor, c_disabledLabelColor};
static_assert(c_textLabels.size() == c_buttonStateCount);

// How a button of an image set of the theme draws its label and its icon. images names the set, as
// in button-browse. BAFunctions draws the image of the set for the states that thmutil does not
// show, which are the disabled state and a focus that Windows hides. icon names the icon set, as in
// icon-folder, or is null.
struct ButtonStyle {
    const wchar_t* images;
    const wchar_t* icon;
    LabelColors labels;
};

constexpr ButtonStyle c_acceptInstallButton = {L"button-accept-install", L"icon-shield", c_outlinedLabels};
constexpr ButtonStyle c_browseButton = {L"button-browse", L"icon-folder", c_outlinedLabels};
constexpr ButtonStyle c_finishButton = {L"button-finish", nullptr, c_outlinedLabels};
constexpr ButtonStyle c_removeButton = {L"button-remove", L"icon-trash", c_removeLabels};
constexpr ButtonStyle c_printButton = {L"button-print", L"icon-print", c_printLabels};
constexpr ButtonStyle c_textButton = {L"button-text", nullptr, c_textLabels};

}  // namespace Theme
}  // namespace Varjo
