/*
This software is licensed under the BSD 2-Clause License
(http://opensource.org/licenses/BSD-2-Clause).

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

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#  pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#  pragma warning(disable : ALL_CPPCORECHECK_WARNINGS)
#endif
#include <boost/format.hpp>
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#include "About.h"

#include "resource.h"
#include "Utility.h"
#include "VersionInfo.h"

#if defined(_MSC_VER)
#  pragma warning(default : ALL_CODE_ANALYSIS_WARNINGS)
#  pragma warning(default : ALL_CPPCORECHECK_WARNINGS)
#endif

namespace About {

namespace detail {

// NOLINTNEXTLINE(cert-err58-cpp,clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
std::wstring appTitle{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HACCEL acceleratorTable{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HWND dialogWindow{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
INT_PTR dialogResult{-1};

static void OnCommand_EditCopy(HWND hDlg)
{
    Utility::CopyToDialogBoxTextClipboard(hDlg);
}

static void OnCommand_OkayButton(HWND hDlg)
{
    dialogResult = IDOK;
    PostMessageW(hDlg, WM_CLOSE, 0, 0);
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
    case ID_EDIT_COPY:
    {
        OnCommand_EditCopy(hDlg);
    }
    break;
    default:
        break;
    }
}

static void OnClose(HWND hDlg)
{
    DestroyWindow(hDlg);
}

static void OnDestroy(HWND hDlg)
{
    dialogWindow = nullptr;
    if (acceleratorTable != nullptr)
    {
        DestroyAcceleratorTable(acceleratorTable);
    }
    auto* const parent{GetParent(hDlg)};
    if (parent == nullptr)
    {
        return;
    }
    auto style{GetWindowLongPtrW(parent, GWL_STYLE)};
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    style = style & (~WS_DISABLED);
    SetWindowLongPtrW(parent, GWL_STYLE, style);
    SetForegroundWindow(parent);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static bool OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam)
{
    using boost::wformat;
    using boost::io::group;
    UNREFERENCED_PARAMETER(hwndFocus);
    UNREFERENCED_PARAMETER(lParam);
    auto* const parent = GetParent(hDlg);
    if (parent == nullptr)
    {
        return false;
    }
    const auto appAndVersionFormat{Utility::loadString(IDS_APP_AND_VERSION_FORMAT)};
    const auto appAndVersionText{str(wformat(appAndVersionFormat) % appTitle.c_str() % VER_PRODUCT_VERSION_STR_W)};
    SetDlgItemTextW(hDlg, IDC_APP_AND_VERSION_STATIC, appAndVersionText.c_str());
    SetDlgItemTextW(hDlg, IDC_COPYRIGHT_INFO_STATIC, VER_LEGAL_COPYRIGHT_STR_W);
    SetDlgItemTextW(hDlg, IDC_DESCRIPTION_STATIC, DESCRIPTION_W);
    auto style{GetWindowLongPtrW(parent, GWL_STYLE)};
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    style = style|WS_DISABLED;
    SetWindowLongPtrW(parent, GWL_STYLE, style);
    Utility::CenterWindow(hDlg);
    ShowWindow(hDlg, SW_NORMAL);
    SetForegroundWindow(hDlg);
    // return TRUE  unless you set the focus to a control
    return true;
}

static LRESULT OnNotify(HWND hDlg, int idFrom, LPNMHDR pNMHDR)
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(idFrom);
    if (pNMHDR == nullptr)
    {
        return 0;
    }
    switch (pNMHDR->code)
    {
    case NM_CLICK:
    case NM_RETURN:
    {
        auto* const pNMLink{reinterpret_cast<PNMLINK>(pNMHDR)};
        const auto* const url{static_cast<LPCWSTR>(pNMLink->item.szUrl)};
        SHELLEXECUTEINFOW ShellExecuteInfo{};
        ShellExecuteInfo.cbSize = sizeof(ShellExecuteInfo);
        ShellExecuteInfo.lpVerb = L"open";
        ShellExecuteInfo.lpFile = url;
        ShellExecuteInfo.nShow = SW_SHOWNORMAL;
        const auto result{ShellExecuteExW(&ShellExecuteInfo)};
        return result;
    }
    default:
        return 0;
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    HANDLE_MSG(hDlg, WM_COMMAND, OnCommand);
    HANDLE_MSG(hDlg, WM_CLOSE, OnClose);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    HANDLE_MSG(hDlg, WM_DESTROY, OnDestroy);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    HANDLE_MSG(hDlg, WM_INITDIALOG, OnInitDialog);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    HANDLE_MSG(hDlg, WM_NOTIFY, OnNotify);
    default:
        return static_cast<INT_PTR>(FALSE);
    }
}

static INT_PTR MessageLoop()
{
    BOOL getMessage{FALSE};
    MSG msg{};
    while (dialogWindow != nullptr && (getMessage = GetMessageW(&msg, dialogWindow, 0, 0)) != FALSE)
    {
        if (getMessage == -1)
        {
            return dialogResult;
        }
        if (TranslateAcceleratorW(dialogWindow, acceleratorTable, &msg) != 0)
        {
            continue;
        }
        if (IsDialogMessageW(dialogWindow, &msg) == FALSE)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return dialogResult;
}

} // namespace detail

INT_PTR Create(HINSTANCE hInstance, HWND parent, const std::wstring& appTitle)
{
    if (hInstance == nullptr)
    {
        return -1;
    }
    if (appTitle.empty())
    {
        return -1;
    }
    detail::appTitle = appTitle;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    detail::dialogWindow = Utility::CreateDialogWithAccelerators(hInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), parent,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
        detail::DlgProc, 0L, MAKEINTRESOURCEW(IDR_ABOUT_DIALOG_ACCELERATORS), detail::acceleratorTable);
    if (detail::dialogWindow == nullptr)
    {
        return -1;
    }
    return detail::MessageLoop();
}

HWND Get()
{
    return detail::dialogWindow;
}

} // namespace About
