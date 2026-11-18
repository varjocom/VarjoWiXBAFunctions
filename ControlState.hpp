// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include <cstddef>

// The states in which the setup draws its controls. Nothing here needs a window.
namespace Varjo
{

// The order is that of the colors in Theme::LabelColors.
enum class ButtonState {
    Default,
    Hover,
    Pressed,
    Focused,
    Disabled,
    // The number of the states. Not a valid state.
    Counter,
};

constexpr size_t c_buttonStateCount = static_cast<size_t>(ButtonState::Counter);

// thmutil shows the image of the first state that applies, in the order pressed, hover and focused.
// It has no disabled image, so BAFunctions draws the disabled image over the image that thmutil drew.
constexpr ButtonState buttonState(bool enabled, bool pressed, bool hovered, bool focused)
{
    if (!enabled) {
        return ButtonState::Disabled;
    }
    if (pressed) {
        return ButtonState::Pressed;
    }
    if (hovered) {
        return ButtonState::Hover;
    }
    return focused ? ButtonState::Focused : ButtonState::Default;
}

// Returns the part of an image name that names the state, as in icon-shield-hover.
constexpr const wchar_t* stateName(ButtonState state)
{
    switch (state) {
        case ButtonState::Hover: return L"hover";
        case ButtonState::Pressed: return L"pressed";
        case ButtonState::Focused: return L"focused";
        case ButtonState::Disabled: return L"disabled";
        default: return L"default";
    }
}

// Returns the state part of the name of the check box image. The check box has one disabled image,
// whether it is checked or not.
constexpr const wchar_t* checkboxStateName(bool enabled, bool checked, bool pressed, bool hovered, bool focused)
{
    if (!enabled) {
        return L"disabled";
    }
    if (checked) {
        return pressed ? L"checked-pressed" : hovered ? L"checked-hover" : focused ? L"checked-focused" : L"checked";
    }
    return pressed ? L"pressed" : hovered ? L"hover" : focused ? L"focused" : L"default";
}

// A focused edit box keeps its focused look under the mouse, because the user types into it.
enum class FieldState {
    Default,
    Hover,
    Focused,
    Disabled,
    // The number of the states. Not a valid state.
    Counter,
};

constexpr size_t c_fieldStateCount = static_cast<size_t>(FieldState::Counter);

constexpr FieldState fieldState(bool enabled, bool focused, bool hovered)
{
    if (!enabled) {
        return FieldState::Disabled;
    }
    if (focused) {
        return FieldState::Focused;
    }
    return hovered ? FieldState::Hover : FieldState::Default;
}

}  // namespace Varjo
