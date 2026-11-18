// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "Controls.hpp"

#include "Drawing.hpp"
#include "Geometry.hpp"
#include "Handles.hpp"
#include "Images.hpp"
#include "LicensePrinting.hpp"
#include "Subclass.hpp"
#include "WindowUtil.hpp"

#include <richedit.h>
#include <uxtheme.h>

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>

namespace Varjo
{
namespace Controls
{
namespace
{

// The icon and the label of a button are centered together between these paddings. A button
// without an icon centers its label.
constexpr int c_iconSize = 18;
constexpr int c_iconGap = 4;
constexpr int c_iconLeftPadding = 12;
constexpr int c_iconRightPadding = 16;

constexpr int c_checkboxSize = 20;

constexpr int c_fieldRadius = 6;
constexpr int c_fieldTextPadding = 12;

struct FieldColors {
    COLORREF background;
    COLORREF border;
    COLORREF text;
};

// The colors of the edit box in each FieldState.
constexpr std::array c_fieldColors{
    FieldColors{Theme::c_fieldBackgroundColor, RGB(0x56, 0x56, 0x56), RGB(0xff, 0xff, 0xff)},
    FieldColors{RGB(0x56, 0x56, 0x56), RGB(0x9c, 0x9c, 0x9c), RGB(0xff, 0xff, 0xff)},
    FieldColors{Theme::c_fieldBackgroundColor, RGB(0xce, 0xce, 0xce), RGB(0xff, 0xff, 0xff)},
    FieldColors{Theme::c_fieldBackgroundColor, RGB(0x56, 0x56, 0x56), RGB(0x56, 0x56, 0x56)},
};
static_assert(c_fieldColors.size() == c_fieldStateCount);

// The rich edit control lies this far inside its box, so that its scroll bar stays inside the
// rounded corners. The text keeps c_licensePadding from the edges of the box, and from the scroll
// bar on the right.
constexpr int c_licenseRadius = 8;
constexpr RECT c_licenseInset = {2, 4, 2, 4};
constexpr RECT c_licensePadding = {16, 12, 4, 12};

// The box around the message of the Failure page.
constexpr COLORREF c_messageBoxBackgroundColor = RGB(0x30, 0x30, 0x30);
constexpr COLORREF c_messageBoxBorderColor = RGB(0x56, 0x56, 0x56);
constexpr int c_messageBoxRadius = 8;
constexpr int c_messageBoxPaddingX = 16;
constexpr int c_messageBoxPaddingY = 12;

// The label lays itself out again after thmutil has moved the controls of the page, which it does
// one control after another.
constexpr UINT c_layOutLabelMessage = WM_APP + 1;

class ImageButton final : public Subclass
{
public:
    ImageButton(HWND button, ImageCache& images, const Theme::ButtonStyle* style, const std::atomic<bool>* showIcon,
        const std::atomic<const Theme::ButtonStyle*>* chosenStyle);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
    std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    LRESULT drawItem(const DRAWITEMSTRUCT& item, WPARAM wParam, LPARAM lParam);

    ImageCache& m_images;
    // m_style is null when m_chosenStyle names the style. m_showIcon is null for a button that shows
    // its icon always.
    const Theme::ButtonStyle* const m_style;
    const std::atomic<bool>* const m_showIcon;
    const std::atomic<const Theme::ButtonStyle*>* const m_chosenStyle;
    // thmutil shows the hover image of the control that its window last saw in WM_SETCURSOR.
    bool m_hovered = false;
    // thmutil draws the text of an owner-drawn button only when the button has text, so the button
    // reports no text while thmutil draws it.
    bool m_hidingText = false;
};

class HoverEnd final : public Subclass
{
public:
    explicit HoverEnd(HWND control);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class Checkbox final : public Subclass
{
public:
    Checkbox(HWND checkbox, ImageCache& images);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    void paint(HDC dc);

    ImageCache& m_images;
    bool m_hovered = false;
};

class EditField final : public Subclass
{
public:
    explicit EditField(HWND edit);

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
    std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    FieldState currentState() const;
    void paintFrame();
    void paintDisabled();

    bool m_hovered = false;
    std::array<UniqueBrush, c_fieldStateCount> m_brushes;
};

class LicenseBox final : public Subclass
{
public:
    explicit LicenseBox(HWND richEdit);

    void setTextRect();

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
    std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    void clearPadding();

    // The rectangle that the theme gives the control, in the client coordinates of the parent.
    RECT m_box = {};
};

class ProgressBar final : public Subclass
{
public:
    explicit ProgressBar(HWND bar);

protected:
    std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    LRESULT paint(const DRAWITEMSTRUCT& item, WPARAM wParam);
};

class HiddenControl final : public Subclass
{
public:
    // hide is null for a control that stays hidden always.
    HiddenControl(HWND control, const std::atomic<bool>* hide);

    bool isHidden() const;

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    const std::atomic<bool>* const m_hide;
};

class FittedLabel final : public Subclass
{
public:
    FittedLabel(HWND label, const LabelLayout& layout);

    void layOut();

protected:
    std::optional<LRESULT> handleMessage(UINT message, WPARAM wParam, LPARAM lParam) override;
    std::optional<LRESULT> handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    bool showsText() const;
    int maxBottom(UINT dpi, int paddingY) const;

    const LabelLayout m_layout;
    bool m_layingOut = false;
    // The rectangle of the box in the client coordinates of the parent. It is empty while no box
    // shows.
    RECT m_box = {};
};

std::wstring imageName(const wchar_t* set, const wchar_t* state);
void reportMouseLeft(HWND control);
int fontHeight(HWND window);
int wrappedTextHeight(HWND label, int width);
void installButton(HWND button, std::unique_ptr<ImageButton> subclass);

}  // namespace

void paintButtonImage(ImageCache& images, HDC dc, const RECT& rect, ButtonState state, const Theme::ButtonStyle& style)
{
    images.draw(dc, imageName(style.images, stateName(state)), rect);
}

void paintButtonLabel(ImageCache& images, HDC dc, const RECT& rect, ButtonState state, const Theme::ButtonStyle& style, std::wstring_view label, HFONT font,
    bool showIcon, UINT dpi)
{
    const SelectedObject selectedFont(dc, font);
    SIZE textSize = {};
    GetTextExtentPoint32W(dc, label.data(), static_cast<int>(label.size()), &textSize);
    const int width = rect.right - rect.left;

    RECT textRect = rect;
    if (showIcon && style.icon) {
        const int iconSize = scale(c_iconSize, dpi);
        const int gap = label.empty() ? 0 : scale(c_iconGap, dpi);
        const int left = rect.left + contentLeft(width, iconSize + gap + textSize.cx, scale(c_iconLeftPadding, dpi), scale(c_iconRightPadding, dpi));
        const int top = rect.top + (rect.bottom - rect.top - iconSize) / 2;
        images.draw(dc, imageName(style.icon, stateName(state)), {left, top, left + iconSize, top + iconSize});
        textRect.left = left + iconSize + gap;
    } else {
        textRect.left = rect.left + (width - textSize.cx) / 2;
    }

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, style.labels[static_cast<size_t>(state)]);
    DrawTextW(dc, label.data(), static_cast<int>(label.size()), &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

void drawButton(ImageCache& images, HWND button, const Theme::ButtonStyle& style)
{
    installButton(button, std::make_unique<ImageButton>(button, images, &style, nullptr, nullptr));
}

void drawButtonWithOptionalIcon(ImageCache& images, HWND button, const Theme::ButtonStyle& style, const std::atomic<bool>& showIcon)
{
    installButton(button, std::make_unique<ImageButton>(button, images, &style, &showIcon, nullptr));
}

void drawButtonInChosenStyle(ImageCache& images, HWND button, const std::atomic<const Theme::ButtonStyle*>& style)
{
    installButton(button, std::make_unique<ImageButton>(button, images, nullptr, nullptr, &style));
}

void endHoverOnLeave(HWND button) { Subclass::install(std::make_unique<HoverEnd>(button)); }

void drawCheckbox(ImageCache& images, HWND checkbox)
{
    // Without visual styles, the button control draws its state changes at once, without the
    // animations that would draw over this subclass later.
    SetWindowTheme(checkbox, L"", L"");
    Subclass::install(std::make_unique<Checkbox>(checkbox, images));
    InvalidateRect(checkbox, nullptr, TRUE);
}

void drawEditField(HWND edit)
{
    const LONG_PTR exStyle = GetWindowLongPtrW(edit, GWL_EXSTYLE);
    SetWindowLongPtrW(edit, GWL_EXSTYLE, exStyle & ~WS_EX_CLIENTEDGE);
    SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
    Subclass::install(std::make_unique<EditField>(edit));
    SetWindowPos(edit, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

void drawLicenseBox(HWND richEdit)
{
    SendMessageW(richEdit, EM_SETBKGNDCOLOR, 0, Theme::c_licenseBackgroundColor);
    setRichEditTextColor(richEdit, Theme::c_textColor);
    // Explorer draws its dark scroll bars with this visual style. Microsoft does not document the
    // name.
    SetWindowTheme(richEdit, L"DarkMode_Explorer", nullptr);

    auto box = std::make_unique<LicenseBox>(richEdit);
    LicenseBox& installed = *box;
    if (!Subclass::install(std::move(box))) {
        return;
    }
    // The control moves into the box that the theme gave it.
    const RECT rect = windowRectInParent(richEdit);
    SetWindowPos(richEdit, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    installed.setTextRect();
}

void drawProgressBar(HWND bar) { Subclass::install(std::make_unique<ProgressBar>(bar)); }

void keepHidden(HWND control)
{
    Subclass::install(std::make_unique<HiddenControl>(control, nullptr));
    ShowWindow(control, SW_HIDE);
}

void keepHidden(HWND control, const std::atomic<bool>& hide)
{
    Subclass::install(std::make_unique<HiddenControl>(control, &hide));
    if (hide) {
        ShowWindow(control, SW_HIDE);
    }
}

void fitLabel(HWND label, const LabelLayout& layout)
{
    auto fitted = std::make_unique<FittedLabel>(label, layout);
    FittedLabel& installed = *fitted;
    if (Subclass::install(std::move(fitted))) {
        installed.layOut();
    }
}

namespace
{

ImageButton::ImageButton(HWND button, ImageCache& images, const Theme::ButtonStyle* style, const std::atomic<bool>* showIcon,
    const std::atomic<const Theme::ButtonStyle*>* chosenStyle)
    : Subclass(button, Scope::WindowAndParent)
    , m_images(images)
    , m_style(style)
    , m_showIcon(showIcon)
    , m_chosenStyle(chosenStyle)
{
}

std::optional<LRESULT> ImageButton::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!m_hidingText) {
        return std::nullopt;
    }
    if (message == WM_GETTEXT) {
        if (wParam > 0) {
            reinterpret_cast<wchar_t*>(lParam)[0] = L'\0';
        }
        return 0;
    }
    if (message == WM_GETTEXTLENGTH) {
        return 0;
    }
    return std::nullopt;
}

// The button control asks its parent to draw it on WM_PAINT and also at once on several state
// changes, so the label is drawn where the parent draws the button.
std::optional<LRESULT> ImageButton::handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_SETCURSOR) {
        m_hovered = reinterpret_cast<HWND>(wParam) == window();
    } else if (message == WM_DRAWITEM) {
        const auto& item = *reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (item.hwndItem == window()) {
            return drawItem(item, wParam, lParam);
        }
    }
    return std::nullopt;
}

LRESULT ImageButton::drawItem(const DRAWITEMSTRUCT& item, WPARAM wParam, LPARAM lParam)
{
    const std::wstring text = windowText(item.hwndItem);
    m_hidingText = true;
    const LRESULT result = callNextOfParent(WM_DRAWITEM, wParam, lParam);
    m_hidingText = false;

    // thmutil shows the focused image also while Windows hides the focus, which it does until the
    // user uses the keyboard.
    const bool enabled = !(item.itemState & ODS_DISABLED);
    const bool pressed = item.itemState & ODS_SELECTED;
    const bool focused = (item.itemState & ODS_FOCUS) && (GetWindowLongPtrW(item.hwndItem, GWL_STYLE) & WS_TABSTOP);
    const ButtonState drawn = buttonState(true, pressed, m_hovered, focused);
    const ButtonState state = buttonState(enabled, pressed, m_hovered, focused && !(item.itemState & ODS_NOFOCUSRECT));
    const Theme::ButtonStyle& style = m_chosenStyle ? **m_chosenStyle : *m_style;
    if (state != drawn || m_chosenStyle) {
        paintButtonImage(m_images, item.hDC, item.rcItem, state, style);
    }
    const bool showIcon = !m_showIcon || *m_showIcon;
    paintButtonLabel(m_images, item.hDC, item.rcItem, state, style, text, windowFont(item.hwndItem), showIcon, windowDpi(item.hwndItem));
    return result;
}

HoverEnd::HoverEnd(HWND control)
    : Subclass(control, Scope::Window)
{
}

std::optional<LRESULT> HoverEnd::handleMessage(UINT message, WPARAM, LPARAM)
{
    if (message == WM_MOUSEMOVE) {
        trackMouseLeave(window());
    } else if (message == WM_MOUSELEAVE) {
        reportMouseLeft(window());
    }
    return std::nullopt;
}

Checkbox::Checkbox(HWND checkbox, ImageCache& images)
    : Subclass(checkbox, Scope::Window)
    , m_images(images)
{
}

std::optional<LRESULT> Checkbox::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_PAINT: {
            PAINTSTRUCT paintStruct = {};
            HDC dc = BeginPaint(window(), &paintStruct);
            paint(dc);
            EndPaint(window(), &paintStruct);
            return 0;
        }
        case WM_PRINTCLIENT: paint(reinterpret_cast<HDC>(wParam)); return 0;
        case WM_ERASEBKGND: return 1;
        case WM_MOUSEMOVE:
            trackMouseLeave(window());
            if (!m_hovered) {
                m_hovered = true;
                InvalidateRect(window(), nullptr, FALSE);
            }
            return std::nullopt;
        case WM_MOUSELEAVE:
            m_hovered = false;
            InvalidateRect(window(), nullptr, FALSE);
            return std::nullopt;
        // The button control draws itself at once on these messages, outside WM_PAINT.
        case BM_SETCHECK:
        case BM_SETSTATE:
        case WM_ENABLE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CAPTURECHANGED:
        case WM_UPDATEUISTATE: {
            const LRESULT result = callNext(message, wParam, lParam);
            paint(WindowDc(window(), WindowDc::Area::Client).get());
            return result;
        }
    }
    return std::nullopt;
}

void Checkbox::paint(HDC dc)
{
    RECT client = {};
    GetClientRect(window(), &client);
    fillRect(dc, client, Theme::c_backgroundColor);

    const int size = scale(c_checkboxSize, windowDpi(window()));
    const int top = (client.bottom - size) / 2;
    const LRESULT state = SendMessageW(window(), BM_GETSTATE, 0, 0);
    const bool focused = GetFocus() == window() && !(SendMessageW(window(), WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS);
    const wchar_t* const name = checkboxStateName(IsWindowEnabled(window()), state & BST_CHECKED, state & BST_PUSHED, m_hovered, focused);
    m_images.draw(dc, imageName(L"checkbox", name), {0, top, size, top + size});
}

EditField::EditField(HWND edit)
    : Subclass(edit, Scope::WindowAndParent)
{
    for (size_t i = 0; i < c_fieldStateCount; ++i) {
        m_brushes[i].reset(CreateSolidBrush(c_fieldColors[i].background));
    }
}

std::optional<LRESULT> EditField::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_NCCALCSIZE:
            if (wParam) {
                RECT& rect = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam)->rgrc[0];
                const int horizontal = scale(c_fieldTextPadding, windowDpi(window()));
                const int vertical = std::max<int>(1, (rect.bottom - rect.top - fontHeight(window())) / 2);
                InflateRect(&rect, -horizontal, -vertical);
                return 0;
            }
            return std::nullopt;
        case WM_NCPAINT: paintFrame(); return 0;
        case WM_PAINT:
            if (!IsWindowEnabled(window())) {
                paintDisabled();
                return 0;
            }
            return std::nullopt;
        // The padding of the field belongs to the text, so it shows the text cursor and takes clicks.
        case WM_NCHITTEST: return HTCLIENT;
        case WM_MOUSEMOVE:
            trackMouseLeave(window());
            if (!m_hovered) {
                m_hovered = true;
                RedrawWindow(window(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
            }
            return std::nullopt;
        case WM_MOUSELEAVE:
            m_hovered = false;
            RedrawWindow(window(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
            return std::nullopt;
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_ENABLE: {
            const LRESULT result = callNext(message, wParam, lParam);
            RedrawWindow(window(), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
            return result;
        }
        case WM_SETFONT: {
            // The edit box sets margins from the font, and the frame already holds the padding.
            const LRESULT result = callNext(message, wParam, lParam);
            SendMessageW(window(), EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
            SetWindowPos(window(), nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
            return result;
        }
    }
    return std::nullopt;
}

std::optional<LRESULT> EditField::handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // The edit box asks for WM_CTLCOLORSTATIC while it is disabled.
    if ((message == WM_CTLCOLOREDIT || message == WM_CTLCOLORSTATIC) && reinterpret_cast<HWND>(lParam) == window()) {
        const auto state = static_cast<size_t>(currentState());
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, c_fieldColors[state].text);
        SetBkColor(dc, c_fieldColors[state].background);
        return reinterpret_cast<LRESULT>(m_brushes[state].get());
    }
    return std::nullopt;
}

FieldState EditField::currentState() const { return fieldState(IsWindowEnabled(window()), GetFocus() == window(), m_hovered); }

void EditField::paintFrame()
{
    RECT windowRect = {};
    GetWindowRect(window(), &windowRect);
    RECT client = {};
    GetClientRect(window(), &client);
    MapWindowPoints(window(), nullptr, reinterpret_cast<POINT*>(&client), 2);
    OffsetRect(&client, -windowRect.left, -windowRect.top);
    OffsetRect(&windowRect, -windowRect.left, -windowRect.top);

    const UINT dpi = windowDpi(window());
    const FieldColors& colors = c_fieldColors[static_cast<size_t>(currentState())];
    const int border = scale(1, dpi);
    const int radius = scale(c_fieldRadius, dpi);
    RECT inner = windowRect;
    InflateRect(&inner, -border, -border);

    const WindowDc dc(window(), WindowDc::Area::Window);
    ExcludeClipRect(dc.get(), client.left, client.top, client.right, client.bottom);
    fillRect(dc.get(), windowRect, Theme::c_backgroundColor);
    fillRoundRects(dc.get(), {{windowRect, static_cast<float>(radius), colors.border}, {inner, static_cast<float>(radius - border), colors.background}});
}

// A disabled edit box draws its text in the gray of the system, whatever color its parent sets.
void EditField::paintDisabled()
{
    PAINTSTRUCT paintStruct = {};
    HDC dc = BeginPaint(window(), &paintStruct);
    RECT client = {};
    GetClientRect(window(), &client);
    const FieldColors& colors = c_fieldColors[static_cast<size_t>(FieldState::Disabled)];
    fillRect(dc, client, colors.background);

    const std::wstring text = windowText(window());
    {
        const SelectedObject selectedFont(dc, windowFont(window()));
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, colors.text);
        DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &client, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    EndPaint(window(), &paintStruct);
}

LicenseBox::LicenseBox(HWND richEdit)
    : Subclass(richEdit, Scope::WindowAndParent)
{
}

void LicenseBox::setTextRect()
{
    RECT client = {};
    GetClientRect(window(), &client);
    const UINT dpi = windowDpi(window());
    const RECT inset = scaleRect(c_licenseInset, dpi);
    const RECT padding = scaleRect(c_licensePadding, dpi);
    RECT text = {client.left + padding.left - inset.left, client.top + padding.top - inset.top, client.right - padding.right,
        client.bottom - padding.bottom + inset.bottom};
    SendMessageW(window(), EM_SETRECT, 0, reinterpret_cast<LPARAM>(&text));
}

std::optional<LRESULT> LicenseBox::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_WINDOWPOSCHANGING: {
            // thmutil places the control at the rectangle of the theme, also when the DPI changes.
            auto& position = *reinterpret_cast<WINDOWPOS*>(lParam);
            if (!(position.flags & (SWP_NOMOVE | SWP_NOSIZE))) {
                m_box = {position.x, position.y, position.x + position.cx, position.y + position.cy};
                const RECT inset = scaleRect(c_licenseInset, windowDpi(window()));
                position.x += inset.left;
                position.y += inset.top;
                position.cx -= inset.left + inset.right;
                position.cy -= inset.top + inset.bottom;
            }
            return std::nullopt;
        }
        case WM_SIZE: {
            const LRESULT result = callNext(message, wParam, lParam);
            setTextRect();
            return result;
        }
        case WM_PAINT: {
            const LRESULT result = callNext(message, wParam, lParam);
            clearPadding();
            return result;
        }
        case WM_SHOWWINDOW: invalidateOutside(m_box, window()); return std::nullopt;
    }
    return std::nullopt;
}

std::optional<LRESULT> LicenseBox::handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_ERASEBKGND) {
        return std::nullopt;
    }
    const LRESULT result = callNextOfParent(message, wParam, lParam);
    if (IsWindowVisible(window())) {
        const auto radius = static_cast<float>(scale(c_licenseRadius, windowDpi(parent())));
        paintBehind(reinterpret_cast<HDC>(wParam), window(), {{m_box, radius, Theme::c_licenseBackgroundColor}});
    }
    return result;
}

// Fills the padding between the formatting rectangle and the edges of the control. The rich edit
// control draws its last line also into the padding below the rectangle. When it scrolls, it moves
// and repaints only the rectangle, so that padding would keep the text of an earlier position.
void LicenseBox::clearPadding()
{
    RECT client = {};
    GetClientRect(window(), &client);
    RECT text = {};
    SendMessageW(window(), EM_GETRECT, 0, reinterpret_cast<LPARAM>(&text));
    const WindowDc dc(window(), WindowDc::Area::Client);
    ExcludeClipRect(dc.get(), text.left, text.top, text.right, text.bottom);
    fillRect(dc.get(), client, Theme::c_licenseBackgroundColor);
}

ProgressBar::ProgressBar(HWND bar)
    : Subclass(bar, Scope::WindowAndParent)
{
}

std::optional<LRESULT> ProgressBar::handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DRAWITEM && reinterpret_cast<const DRAWITEMSTRUCT*>(lParam)->hwndItem == window()) {
        return paint(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam), wParam);
    }
    return std::nullopt;
}

// thmutil keeps the progress to itself, so it draws the bar into a bitmap first. The bitmap shows
// how much of the bar is filled, and in which colors.
LRESULT ProgressBar::paint(const DRAWITEMSTRUCT& item, WPARAM wParam)
{
    const int width = item.rcItem.right - item.rcItem.left;
    const int height = item.rcItem.bottom - item.rcItem.top;
    LRESULT result = 0;
    COLORREF filled = 0;
    COLORREF unfilled = 0;
    int filledColumns = 0;
    {
        const UniqueDc memory(CreateCompatibleDC(item.hDC));
        const UniqueBitmap bitmap(CreateCompatibleBitmap(item.hDC, width, height));
        const SelectedObject selectedBitmap(memory.get(), bitmap.get());
        DRAWITEMSTRUCT copy = item;
        copy.hDC = memory.get();
        copy.rcItem = {0, 0, width, height};
        result = callNextOfParent(WM_DRAWITEM, wParam, reinterpret_cast<LPARAM>(&copy));

        const int row = height / 2;
        filled = GetPixel(memory.get(), 0, row);
        unfilled = GetPixel(memory.get(), width - 1, row);
        while (filledColumns < width && GetPixel(memory.get(), filledColumns, row) == filled) {
            ++filledColumns;
        }
    }

    // The rest of the bar starts under the round end of the filled part. The left end of the filled
    // part then lies on the background alone, without an edge of the rest showing around it.
    fillRect(item.hDC, item.rcItem, Theme::c_backgroundColor);
    const int fill = progressFillWidth(filledColumns, width);
    const RECT rest = {item.rcItem.left + std::max(0, fill - height), item.rcItem.top, item.rcItem.right, item.rcItem.bottom};
    if (fill > 0) {
        const RECT filledRect = {item.rcItem.left, item.rcItem.top, item.rcItem.left + fill, item.rcItem.bottom};
        fillRoundRects(item.hDC, {{rest, height / 2.0f, unfilled}, {filledRect, std::min(height, fill) / 2.0f, filled}});
    } else {
        fillRoundRects(item.hDC, {{rest, height / 2.0f, unfilled}});
    }
    return result;
}

HiddenControl::HiddenControl(HWND control, const std::atomic<bool>* hide)
    : Subclass(control, Scope::Window)
    , m_hide(hide)
{
}

bool HiddenControl::isHidden() const { return !m_hide || *m_hide; }

std::optional<LRESULT> HiddenControl::handleMessage(UINT message, WPARAM, LPARAM lParam)
{
    if (message == WM_WINDOWPOSCHANGING && isHidden()) {
        reinterpret_cast<WINDOWPOS*>(lParam)->flags &= ~SWP_SHOWWINDOW;
    }
    return std::nullopt;
}

FittedLabel::FittedLabel(HWND label, const LabelLayout& layout)
    : Subclass(label, Scope::WindowAndParent)
    , m_layout(layout)
{
}

void FittedLabel::layOut()
{
    if (m_layingOut) {
        return;
    }
    m_layingOut = true;
    const UINT dpi = windowDpi(window());
    const int paddingX = m_layout.inBox ? scale(c_messageBoxPaddingX, dpi) : 0;
    const int paddingY = m_layout.inBox ? scale(c_messageBoxPaddingY, dpi) : 0;
    RECT rect = windowRectInParent(window());
    const RECT oldBox = m_box;
    // The theme leaves the gap above the label or its box, so next takes its top.
    int nextTop = rect.top - paddingY;
    if (showsText()) {
        rect.bottom = rect.top + wrappedTextHeight(window(), rect.right - rect.left);
        rect.bottom = std::max(rect.top, std::min<LONG>(rect.bottom, maxBottom(dpi, paddingY)));
        SetWindowPos(window(), nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        const int bottom = rect.bottom + paddingY;
        if (m_layout.inBox) {
            m_box = {rect.left - paddingX, rect.top - paddingY, rect.right + paddingX, bottom};
        }
        nextTop = bottom + scale(m_layout.gap, dpi);
    } else {
        m_box = {};
    }
    const RECT next = windowRectInParent(m_layout.next);
    SetWindowPos(m_layout.next, nullptr, next.left, nextTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    invalidateOutside(oldBox, window());
    invalidateOutside(m_box, window());
    InvalidateRect(window(), nullptr, TRUE);
    m_layingOut = false;
}

std::optional<LRESULT> FittedLabel::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_SETTEXT: {
            const LRESULT result = callNext(message, wParam, lParam);
            layOut();
            return result;
        }
        case WM_WINDOWPOSCHANGED: {
            const UINT flags = reinterpret_cast<const WINDOWPOS*>(lParam)->flags;
            const bool changed = (flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) || !(flags & SWP_NOSIZE) || !(flags & SWP_NOMOVE);
            if (changed && !m_layingOut) {
                PostMessageW(window(), c_layOutLabelMessage, 0, 0);
            }
            return std::nullopt;
        }
        case c_layOutLabelMessage: layOut(); return 0;
    }
    return std::nullopt;
}

std::optional<LRESULT> FittedLabel::handleParentMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_ERASEBKGND) {
        return std::nullopt;
    }
    const LRESULT result = callNextOfParent(message, wParam, lParam);
    if (!IsRectEmpty(&m_box) && showsText()) {
        const UINT dpi = windowDpi(parent());
        const auto radius = static_cast<float>(scale(c_messageBoxRadius, dpi));
        const int border = scale(1, dpi);
        RECT inner = m_box;
        InflateRect(&inner, -border, -border);
        paintBehind(reinterpret_cast<HDC>(wParam), window(), {{m_box, radius, c_messageBoxBorderColor}, {inner, radius - border, c_messageBoxBackgroundColor}});
    }
    return result;
}

bool FittedLabel::showsText() const { return hasVisibleStyle(window()) && GetWindowTextLengthW(window()) > 0; }

// Returns the lowest bottom of the label that keeps next above the bound.
int FittedLabel::maxBottom(UINT dpi, int paddingY) const
{
    const RECT next = windowRectInParent(m_layout.next);
    const int limit = windowRectInParent(m_layout.bound).top - scale(m_layout.boundMargin, dpi);
    return limit - (next.bottom - next.top) - scale(m_layout.gap, dpi) - paddingY;
}

std::wstring imageName(const wchar_t* set, const wchar_t* state) { return std::wstring(set) + L"-" + state; }

// Tells thmutil which control the mouse has moved to from control. thmutil sees the mouse only
// through WM_SETCURSOR of the window, which neither another window nor a rich edit control sends
// it. Telling it again about a control that has sent WM_SETCURSOR changes nothing.
void reportMouseLeft(HWND control)
{
    HWND parent = GetParent(control);
    POINT cursor = {};
    GetCursorPos(&cursor);
    HWND under = WindowFromPoint(cursor);
    if (under == control) {
        return;
    }
    HWND target = under == parent || IsChild(parent, under) ? under : parent;
    SendMessageW(parent, WM_SETCURSOR, reinterpret_cast<WPARAM>(target), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
}

int fontHeight(HWND window)
{
    const WindowDc dc(window, WindowDc::Area::Client);
    const SelectedObject selectedFont(dc.get(), windowFont(window));
    TEXTMETRICW metrics = {};
    GetTextMetricsW(dc.get(), &metrics);
    return metrics.tmHeight;
}

int wrappedTextHeight(HWND label, int width)
{
    const std::wstring text = windowText(label);
    const WindowDc dc(label, WindowDc::Area::Client);
    const SelectedObject selectedFont(dc.get(), windowFont(label));
    RECT rect = {0, 0, width, 0};
    DrawTextW(dc.get(), text.c_str(), static_cast<int>(text.size()), &rect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    return rect.bottom;
}

void installButton(HWND button, std::unique_ptr<ImageButton> subclass)
{
    Subclass::install(std::move(subclass));
    endHoverOnLeave(button);
}

}  // namespace
}  // namespace Controls
}  // namespace Varjo
