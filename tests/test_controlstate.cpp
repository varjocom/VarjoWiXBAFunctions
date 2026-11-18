// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "ControlState.hpp"

#include <gtest/gtest.h>

#include <string_view>

using namespace Varjo;

TEST(ControlStateTest, disabledButtonShouldIgnoreTheMouseAndTheFocus) { EXPECT_EQ(ButtonState::Disabled, buttonState(false, true, true, true)); }

TEST(ControlStateTest, buttonStatesShouldFollowTheOrderOfThmutil)
{
    EXPECT_EQ(ButtonState::Pressed, buttonState(true, true, true, true));
    EXPECT_EQ(ButtonState::Hover, buttonState(true, false, true, true));
    EXPECT_EQ(ButtonState::Focused, buttonState(true, false, false, true));
    EXPECT_EQ(ButtonState::Default, buttonState(true, false, false, false));
}

TEST(ControlStateTest, stateNamesShouldMatchTheImageFiles)
{
    EXPECT_EQ(std::wstring_view(L"default"), stateName(ButtonState::Default));
    EXPECT_EQ(std::wstring_view(L"hover"), stateName(ButtonState::Hover));
    EXPECT_EQ(std::wstring_view(L"pressed"), stateName(ButtonState::Pressed));
    EXPECT_EQ(std::wstring_view(L"focused"), stateName(ButtonState::Focused));
    EXPECT_EQ(std::wstring_view(L"disabled"), stateName(ButtonState::Disabled));
}

TEST(ControlStateTest, disabledCheckboxShouldHaveOneImage)
{
    EXPECT_EQ(std::wstring_view(L"disabled"), checkboxStateName(false, true, true, true, true));
    EXPECT_EQ(std::wstring_view(L"disabled"), checkboxStateName(false, false, false, false, false));
}

TEST(ControlStateTest, uncheckedCheckboxShouldShowItsState)
{
    EXPECT_EQ(std::wstring_view(L"pressed"), checkboxStateName(true, false, true, true, true));
    EXPECT_EQ(std::wstring_view(L"hover"), checkboxStateName(true, false, false, true, true));
    EXPECT_EQ(std::wstring_view(L"focused"), checkboxStateName(true, false, false, false, true));
    EXPECT_EQ(std::wstring_view(L"default"), checkboxStateName(true, false, false, false, false));
}

TEST(ControlStateTest, checkedCheckboxShouldShowItsState)
{
    EXPECT_EQ(std::wstring_view(L"checked-pressed"), checkboxStateName(true, true, true, true, true));
    EXPECT_EQ(std::wstring_view(L"checked-hover"), checkboxStateName(true, true, false, true, true));
    EXPECT_EQ(std::wstring_view(L"checked-focused"), checkboxStateName(true, true, false, false, true));
    EXPECT_EQ(std::wstring_view(L"checked"), checkboxStateName(true, true, false, false, false));
}

TEST(ControlStateTest, focusedFieldShouldKeepItsLookUnderTheMouse)
{
    EXPECT_EQ(FieldState::Focused, fieldState(true, true, true));
    EXPECT_EQ(FieldState::Hover, fieldState(true, false, true));
    EXPECT_EQ(FieldState::Default, fieldState(true, false, false));
    EXPECT_EQ(FieldState::Disabled, fieldState(false, true, true));
}
