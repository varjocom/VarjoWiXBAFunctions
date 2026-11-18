// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#pragma once

#include "ControlState.hpp"
#include "Theme.hpp"

#include <windows.h>

#include <atomic>
#include <string_view>

namespace Varjo
{

class ImageCache;

// Drawing that the theme of the setup cannot express. Each function that takes a control subclasses
// it, and the subclass lives as long as the control. All of them run on the thread of the window.
// Lengths are pixels at 96 DPI. The image cache must outlive the controls that use it.
namespace Controls
{

// Draws the image of the state from the image set of style into rect.
void paintButtonImage(ImageCache& images, HDC dc, const RECT& rect, ButtonState state, const Theme::ButtonStyle& style);

// Draws the icon of style and the label in the color of the state, centered together in rect, which
// is the rectangle of the button. showIcon false leaves the icon out. dpi is the DPI of the button.
void paintButtonLabel(ImageCache& images, HDC dc, const RECT& rect, ButtonState state, const Theme::ButtonStyle& style, std::wstring_view label, HFONT font,
    bool showIcon, UINT dpi);

// Draws the label and the icon of an image button of the theme, and its disabled image, over the
// image that thmutil draws.
void drawButton(ImageCache& images, HWND button, const Theme::ButtonStyle& style);

// Draws the button as drawButton does, and shows the icon only while showIcon is true. showIcon must
// outlive the button.
void drawButtonWithOptionalIcon(ImageCache& images, HWND button, const Theme::ButtonStyle& style, const std::atomic<bool>& showIcon);

// Draws an image button of the theme in the style that style names when the button paints. The
// image of every state comes from that style rather than from the theme. style must outlive the
// button.
void drawButtonInChosenStyle(ImageCache& images, HWND button, const std::atomic<const Theme::ButtonStyle*>& style);

// Ends the hover state of an image button of the theme when the mouse leaves the button. thmutil
// ends it only when the mouse moves onto a part of the window that reports the mouse to it, which
// another window and a rich edit control do not. The drawButton functions include this.
void endHoverOnLeave(HWND button);

// Draws the check box from the checkbox images of the theme. The system draws a light box, and it
// ignores the text color of the theme, so the theme gives the check box no text.
void drawCheckbox(ImageCache& images, HWND checkbox);

// Draws a single-line edit box as a rounded field with a border in the colors of its state, and
// centers its text vertically.
void drawEditField(HWND edit);

// Shows a rich edit control in the colors of the theme, with a dark scroll bar, inside a rounded box.
// The box takes the rectangle that the theme gives the control, and the control shrinks into the
// box. The text keeps a padding from the box.
void drawLicenseBox(HWND richEdit);

// Draws an image progress bar of the theme with rounded ends. Its image has four columns. The first
// two have the color of the filled part, and the last two another color for the rest of the bar.
void drawProgressBar(HWND bar);

// Keeps the control hidden, also when thmutil shows the page of the control.
void keepHidden(HWND control);

// Keeps the control hidden while hide is true. hide must outlive the control.
void keepHidden(HWND control, const std::atomic<bool>& hide);

// How fitLabel places a label and the control below it.
struct LabelLayout {
    // The control that moves gap pixels below the label. With inBox, BAFunctions draws the label in
    // a rounded box, and next keeps the gap below the box. While the label is hidden or empty, next
    // takes the place of the label or of its box.
    HWND next;
    int gap;
    bool inBox;
    // The bottom of next stays boundMargin pixels above the top of bound, and a longer text is cut
    // off. bound has the same parent as the label.
    HWND bound;
    int boundMargin;
};

// Sizes a label of the theme to the height of its text whenever the text changes, the label is shown
// or thmutil moves it, and places the controls of layout. The controls must outlive the label.
void fitLabel(HWND label, const LabelLayout& layout);

}  // namespace Controls
}  // namespace Varjo
