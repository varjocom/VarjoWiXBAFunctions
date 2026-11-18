// Copyright 2026 Varjo Technologies Oy. Licensed under the Microsoft Reciprocal License (MS-RL).
// LICENSE.txt has the license text.

#include "LicensePrinting.hpp"

#include "Handles.hpp"

#include <commdlg.h>
#include <richedit.h>

#include <algorithm>

namespace Varjo
{
namespace
{

constexpr int c_twipsPerInch = 1440;
constexpr int c_marginTwips = 1080;

RECT printArea(HDC printer);
void print(HDC printer, HWND richEdit, const wchar_t* documentName);

}  // namespace

void setRichEditTextColor(HWND richEdit, COLORREF color)
{
    CHARFORMAT2W format = {};
    format.cbSize = sizeof(format);
    format.dwMask = CFM_COLOR;
    format.crTextColor = color;
    SendMessageW(richEdit, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&format));
}

void printRichEdit(HWND owner, HWND richEdit, const wchar_t* documentName, COLORREF screenColor)
{
    PRINTDLGW dialog = {sizeof(dialog)};
    dialog.hwndOwner = owner;
    dialog.Flags = PD_RETURNDC | PD_NOPAGENUMS | PD_NOSELECTION | PD_USEDEVMODECOPIESANDCOLLATE;
    if (!PrintDlgW(&dialog)) {
        return;
    }
    const UniqueDc printer(dialog.hDC);
    const UniqueGlobalMemory devMode(dialog.hDevMode);
    const UniqueGlobalMemory devNames(dialog.hDevNames);

    SendMessageW(richEdit, WM_SETREDRAW, FALSE, 0);
    setRichEditTextColor(richEdit, RGB(0, 0, 0));
    print(printer.get(), richEdit, documentName);
    setRichEditTextColor(richEdit, screenColor);
    SendMessageW(richEdit, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(richEdit, nullptr, TRUE);
}

namespace
{

// Returns the printable area of the page in twips, relative to the origin of the printer DC, which
// is the top left corner of the printable area. The margins are measured from the edge of the paper.
RECT printArea(HDC printer)
{
    const int dpiX = GetDeviceCaps(printer, LOGPIXELSX);
    const int dpiY = GetDeviceCaps(printer, LOGPIXELSY);
    const int offsetX = MulDiv(GetDeviceCaps(printer, PHYSICALOFFSETX), c_twipsPerInch, dpiX);
    const int offsetY = MulDiv(GetDeviceCaps(printer, PHYSICALOFFSETY), c_twipsPerInch, dpiY);
    const int paperWidth = MulDiv(GetDeviceCaps(printer, PHYSICALWIDTH), c_twipsPerInch, dpiX);
    const int paperHeight = MulDiv(GetDeviceCaps(printer, PHYSICALHEIGHT), c_twipsPerInch, dpiY);

    RECT area = {};
    area.left = std::max(0, c_marginTwips - offsetX);
    area.top = std::max(0, c_marginTwips - offsetY);
    area.right = paperWidth - c_marginTwips - offsetX;
    area.bottom = paperHeight - c_marginTwips - offsetY;
    return area;
}

void print(HDC printer, HWND richEdit, const wchar_t* documentName)
{
    DOCINFOW document = {sizeof(document)};
    document.lpszDocName = documentName && *documentName ? documentName : nullptr;
    if (StartDocW(printer, &document) <= 0) {
        return;
    }

    FORMATRANGE range = {};
    range.hdc = printer;
    range.hdcTarget = printer;
    range.rc = printArea(printer);
    range.rcPage = {0, 0, MulDiv(GetDeviceCaps(printer, HORZRES), c_twipsPerInch, GetDeviceCaps(printer, LOGPIXELSX)),
        MulDiv(GetDeviceCaps(printer, VERTRES), c_twipsPerInch, GetDeviceCaps(printer, LOGPIXELSY))};

    GETTEXTLENGTHEX lengthRequest = {GTL_NUMCHARS | GTL_PRECISE, 1200};
    const LONG length = static_cast<LONG>(SendMessageW(richEdit, EM_GETTEXTLENGTHEX, reinterpret_cast<WPARAM>(&lengthRequest), 0));

    bool failed = false;
    range.chrg = {0, -1};
    while (range.chrg.cpMin < length && !failed) {
        failed = StartPage(printer) <= 0;
        if (!failed) {
            const RECT area = range.rc;
            const LONG next = static_cast<LONG>(SendMessageW(richEdit, EM_FORMATRANGE, TRUE, reinterpret_cast<LPARAM>(&range)));
            range.rc = area;
            failed = EndPage(printer) <= 0 || next <= range.chrg.cpMin;
            range.chrg.cpMin = next;
        }
    }
    SendMessageW(richEdit, EM_FORMATRANGE, FALSE, 0);

    if (failed) {
        AbortDoc(printer);
    } else {
        EndDoc(printer);
    }
}

}  // namespace
}  // namespace Varjo
