/*
Copyright © 2023 - 2026 by Benilda Key

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "stdafx.h"
#include <array>
#include <commdlg.h>

#include "mingw_wil_compat.h"

#include "OptionsDialog.h"

#include "resource.h"

#include "Options.h"
#include "Utility.h"

namespace OptionsDialog {

namespace detail {

// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF white{RGB(0xFF,0xFF,0xFF)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF aqua{RGB(0,0xFF,0xFF)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF fuchsia{RGB(0xFF,0,0xFF)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF yellow{RGB(0xFF,0xFF,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF silver{RGB(0xC0,0xC0,0xC0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF red{RGB(0xFF,0,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF lime{RGB(0,0xFF,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF blue{RGB(0,0,0xFF)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF gray{RGB(0x80,0x80,0x80)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF teal{RGB(0,0x80,0x80)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF purple{RGB(0x80, 0, 0x80)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF olive{RGB(0x80,0x80,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF black{RGB(0,0,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF maroon{RGB(0x80,0,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF green{RGB(0,0x80,0)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF navy{RGB(0,0,0x80)};

// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-avoid-non-const-global-variables,readability-magic-numbers)
std::array<COLORREF, 16> customColors{{ black, maroon, green, olive, navy, purple, teal, silver, gray, red, lime, yellow, blue, fuchsia, aqua, white }};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HINSTANCE hInstance{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
COLORREF backgroundColor{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
COLORREF clockColor{0};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HWND currentBackgroundColorStatic{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HWND currentClockColorStatic{};

static COLORREF ChooseColor(HWND hDlg, COLORREF currentColor)
{
    CHOOSECOLORW chooseColorData{};
    chooseColorData.lStructSize = sizeof(CHOOSECOLORW);
    chooseColorData.hwndOwner = hDlg;
    chooseColorData.hInstance = nullptr;
    chooseColorData.rgbResult = currentColor;
    chooseColorData.lpCustColors = customColors.data();
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    chooseColorData.Flags = CC_RGBINIT|CC_FULLOPEN;
    chooseColorData.lCustData = 0;
    chooseColorData.lpfnHook = nullptr;
    chooseColorData.lpTemplateName = nullptr;
    const auto result = ::ChooseColorW(&chooseColorData);
    if (result == FALSE)
    {
        return currentColor;
    }
    return chooseColorData.rgbResult;
}

static bool InitChildWindowHandles(HWND hDlg)
{
    currentBackgroundColorStatic = GetDlgItem(hDlg, IDC_CURRENT_BACKGROUND_COLOR_STATIC);
    currentClockColorStatic = GetDlgItem(hDlg, IDC_CURRENT_CLOCK_COLOR_STATIC);
    return (currentBackgroundColorStatic != nullptr && currentClockColorStatic != nullptr);
}

static void InitDialogData()
{
    backgroundColor = Options::GetBackgroundColor();
    clockColor = Options::GetClockColor();
}

static LRESULT OnPaintOwnerDrawStatic(HWND hWnd)
{
    if (hWnd == nullptr || IsWindow(hWnd) == FALSE)
    {
        return 0;
    }
    wil_compat::BeginPaint_context paint(hWnd);
    if (!paint.get())
    {
        return 0;
    }
    RECT clientRect{};
    if (GetClientRect(hWnd, &clientRect) == FALSE)
    {
        return 0;
    }
    const auto id{GetDlgCtrlID(hWnd)};
    COLORREF brushColor{0};
    switch (id)
    {
    case IDC_CURRENT_BACKGROUND_COLOR_STATIC:
    {
        brushColor = backgroundColor;
    }
    break;
    case IDC_CURRENT_CLOCK_COLOR_STATIC:
    {
        brushColor = clockColor;
    }
    break;
    default:
        return 0;
    }
    wil_compat::unique_hbrush brush(CreateSolidBrush(brushColor));
    FillRect(paint.get(), &clientRect, brush.get());
    return 0;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static LRESULT CALLBACK OwnerDrawStaticProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, [[maybe_unused]] UINT_PTR uIdSubclass,
    [[maybe_unused]] DWORD_PTR dwRefData)
{
    // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
    switch (uMsg)
    {
    HANDLE_MSG(hWnd, WM_PAINT, OnPaintOwnerDrawStatic);
    default:
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}

static bool SubclassChildWindows([[maybe_unused]] HWND hDlg)
{
    return (SetWindowSubclass(currentBackgroundColorStatic, OwnerDrawStaticProc, 0, 0) != FALSE
        && SetWindowSubclass(currentClockColorStatic, OwnerDrawStaticProc, 0, 0) != FALSE);
}

static void OnCommand_OkayButton(HWND hDlg)
{
    Options::SetBackgroundColor(backgroundColor);
    Options::SetClockColor(clockColor);
    Options::Save();
    EndDialog(hDlg, IDOK);
}

static void OnCommand_CancelButton(HWND hDlg)
{
    EndDialog(hDlg, IDCANCEL);
}

static void OnCommand_ChooseBackgroundColorButton(HWND hDlg)
{
    backgroundColor = ChooseColor(hDlg, backgroundColor);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RedrawWindow(currentBackgroundColorStatic, nullptr, nullptr, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
}

static void OnCommand_ChooseClockColorButton(HWND hDlg)
{
    clockColor = ChooseColor(hDlg, clockColor);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RedrawWindow(currentClockColorStatic, nullptr, nullptr, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
}

static void OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);
    switch (id)
    {
    case IDOK:
    {
        OnCommand_OkayButton(hDlg);
    }
    break;
    case IDCANCEL:
    {
        OnCommand_CancelButton(hDlg);
    }
    break;
    case IDC_CHOOSE_BACKGROUND_COLOR_BUTTON:
    {
        OnCommand_ChooseBackgroundColorButton(hDlg);
    }
    break;
    case IDC_CHOOSE_CLOCK_COLOR_BUTTON:
    {
        OnCommand_ChooseClockColorButton(hDlg);
    }
    break;
    default:
        break;
    }
}

static void OnDestroy([[maybe_unused]] HWND hWnd)
{
    RemoveWindowSubclass(currentBackgroundColorStatic, OwnerDrawStaticProc, 0);
    RemoveWindowSubclass(currentClockColorStatic, OwnerDrawStaticProc, 0);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static bool OnInitDialog(HWND hDlg, [[maybe_unused]] HWND hwndFocus, [[maybe_unused]] LPARAM lParam)
{
    if (!InitChildWindowHandles(hDlg))
    {
        return false;
    }
    if (!SubclassChildWindows(hDlg))
    {
        return false;
    }
    InitDialogData();
    Utility::CenterWindow(hDlg);
    ShowWindow(hDlg, SW_NORMAL);
    SetForegroundWindow(hDlg);
    // return TRUE  unless you set the focus to a control
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    HANDLE_MSG(hDlg, WM_COMMAND, OnCommand);
    HANDLE_MSG(hDlg, WM_DESTROY, OnDestroy);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    HANDLE_MSG(hDlg, WM_INITDIALOG, OnInitDialog);
    default:
        return static_cast<INT_PTR>(FALSE);
    }
}

} // namespace detail

INT_PTR Create(HINSTANCE hInstance, HWND parent)
{
    if (hInstance == nullptr)
    {
        return -1;
    }
    detail::hInstance = hInstance;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    return DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_OPTIONS_DIALOG_BOX), parent, detail::DlgProc, 0L);
}

} // namespace OptionsDialog
