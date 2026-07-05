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

#include "App.h"

#include "resource.h"
#include "SystemTray.h"
#include "Utility.h"
#include "VersionInfo.h"
#include "Window.h"

#if defined(_MSC_VER)
// NOLINTNEXTLINE(clang-diagnostic-unknown-pragmas)
#  pragma execution_character_set("utf-8")
#endif

namespace App {

namespace detail {

const UINT SystemTrayIconId{110};
// Was COM initialized.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
bool COM_initialized{false};
// The title bar text.
// NOLINTNEXTLINE(cert-err58-cpp,clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
std::wstring title{};

/**
Creates main window.

@param[in] hInstance
  The handle to the current instance.
@return
  true if successful.
  false otherwise.
*/
static bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    auto createWindow{Window::Create(hInstance, title)};
    if (!createWindow)
    {
        return false;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    SystemTray::CreateSystemTrayIcon(Window::Get(), MAKEINTRESOURCEW(IDI_APP), title.c_str(), SystemTrayIconId,
		Utility::to_underlying(Window::appMessages::WM_SYSTEM_TRAY_CALLBACK));
    ShowWindow(Window::Get(), nCmdShow);
    UpdateWindow(Window::Get());
    return true;
}

} // namespace detail

/**
Performs the initialization tasks.

@param[in] hInstance
  The handle to the current instance.
@return
  true if successful.
  false otherwise.
*/
bool Init(HINSTANCE hInstance, int nCmdShow)
{
    const DWORD ApplicationReservedLastShutdownRange{0x100};
    Utility::SetResouceInstance(hInstance);
    _wsetlocale(LC_ALL, L".UTF-8");
    SetProcessShutdownParameters(ApplicationReservedLastShutdownRange, SHUTDOWN_NORETRY);
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls{};
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    InitCtrls.dwICC = ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_USEREX_CLASSES|ICC_COOL_CLASSES|ICC_INTERNET_CLASSES
		|ICC_PAGESCROLLER_CLASS|ICC_NATIVEFNTCTL_CLASS|ICC_STANDARD_CLASSES|ICC_LINK_CLASS;
    auto initRes{InitCommonControlsEx(&InitCtrls)};
    if (initRes == FALSE)
    {
        Utility::messageBox(nullptr, IDS_COMMON_CONTROLS_INIT_FAILED, IDS_ERROR, MB_OK);
        return FALSE;
    }
    if (FAILED(::CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
    {
        Utility::messageBox(nullptr, IDS_OLE_INIT_FAILED, IDS_ERROR, MB_OK);
        return false;
    }
    detail::COM_initialized = true;
    detail::title = Utility::loadString(hInstance, IDS_APP_TITLE);
    if (detail::title.empty())
    {
        detail::title = VER_APP_NAME_STR_W;
    }
    if (detail::title.empty())
    {
        return false;
    }
    return detail::InitInstance(hInstance, nCmdShow);
}

void Cleanup()
{
    if (detail::COM_initialized)
    {
        ::CoUninitialize();
        detail::COM_initialized = false;
    }
}

int Run()
{
    int ret{Window::MessageLoop()};
    Cleanup();
    return ret;
}

} // namespace App
