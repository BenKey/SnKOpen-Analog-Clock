/*
Copyright © 2019 - 2026 by Benilda Key

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

#include <memory>

#include "mingw_wil_compat.h"

#include "SystemTray.h"

#include "Utility.h"


namespace SystemTray {

namespace detail {

// Data for the system tray icon.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
NOTIFYICONDATAW iconData{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool iconCreated{false};

static DWORD GetNotifyIconDataSize()
{
    DWORD ret{0};
    const auto shell32Version{Utility::GetDllVersion(L"Shell32.dll")};
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    if (shell32Version >= MAKEDLLVERULL(6, 0, 6, 0))
    {
        ret = sizeof(NOTIFYICONDATAW);
    }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    else if (shell32Version >= MAKEDLLVERULL(6, 0, 0, 0))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        ret = NOTIFYICONDATAW_V3_SIZE;
    }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    else if (shell32Version >= MAKEDLLVERULL(5, 0, 0, 0))
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        ret = NOTIFYICONDATAW_V2_SIZE;
    }
    else
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        ret = NOTIFYICONDATAW_V1_SIZE;
    }
    return ret;
}

static DWORD GetNotifyIconDataVersion()
{
    DWORD ret{0};
    const auto shell32Version{Utility::GetDllVersion(L"Shell32.dll")};
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    if (shell32Version >= MAKEDLLVERULL(6, 0, 6, 0))
    {
        ret = NOTIFYICON_VERSION_4;
    }
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    else if (shell32Version >= MAKEDLLVERULL(6, 0, 0, 0))
    {
        ret = NOTIFYICON_VERSION;
    }
    return ret;
}

} // namespace detail

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool CreateSystemTrayIcon(HWND window, LPCWSTR iconName, LPCWSTR title, UINT uID, UINT uCallbackMessage)
{
    if (detail::iconCreated)
    {
        return true;
    }
    if (window == nullptr)
    {
        return false;
    }
    if (iconName == nullptr)
    {
        return false;
    }
    if (title == nullptr)
    {
        return false;
    }
    if (uID == 0)
    {
        return false;
    }
    if (uCallbackMessage == 0)
    {
        return false;
    }
    if (Utility::GetTaskbarWindow() == nullptr)
    {
        return false;
    }
    detail::iconData.cbSize = detail::GetNotifyIconDataSize();
    detail::iconData.hWnd = window;
    detail::iconData.uID = uID;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    detail::iconData.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP;
    detail::iconData.uCallbackMessage = uCallbackMessage;
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    wil_compat::unique_hicon iconPtr(reinterpret_cast<HICON>(Utility::loadImage(iconName, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE|LR_SHARED)));
    detail::iconData.hIcon = iconPtr.get();
    if (detail::iconData.hIcon == nullptr)
    {
        return false;
    }
    StringCchCopyW(static_cast<LPWSTR>(detail::iconData.szTip), _countof(detail::iconData.szTip), title);
    StringCchCopyW(static_cast<LPWSTR>(detail::iconData.szInfo), _countof(detail::iconData.szInfo), title);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    detail::iconData.uVersion = detail::GetNotifyIconDataVersion();
    const auto notifyIcon = Shell_NotifyIconW(NIM_ADD, &detail::iconData);
    if (notifyIcon == FALSE)
    {
        return false;
    }
    detail::iconCreated = true;
    return true;
}

bool DeleteSystemTrayIcon()
{
    if (!detail::iconCreated)
    {
        return false;
    }
    const auto notifyIcon{Shell_NotifyIconW(NIM_DELETE, &detail::iconData)};
    SecureZeroMemory(&detail::iconData, sizeof(detail::iconData));
    if (notifyIcon == FALSE)
    {
        return false;
    }
    detail::iconCreated = false;
    return true;
}

} // namespace SystemTray
