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

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <numbers>
#include <string>
#include <vector>

#include <datetimeapi.h>

#include <gsl/gsl>
#include "mingw_wil_compat.h"

#include "resource.h"

#include "Window.h"

#include "About.h"
#include "DirectAnnotation.h"
#include "Options.h"
#include "OptionsDialog.h"
#include "Utility.h"

#if defined(_MSC_VER)
#  pragma warning(default : ALL_CODE_ANALYSIS_WARNINGS)
#  pragma warning(default : ALL_CPPCORECHECK_WARNINGS)
#endif

using namespace std::string_literals;

namespace Window {

namespace detail {

constexpr size_t smallBuffer{64};
constexpr UINT_PTR ID_TIMER{1};
constexpr UINT TimerElapse{500};
constexpr double TWOPI{2 * std::numbers::pi};
constexpr int clockPenWidth{6};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HINSTANCE hInstance{nullptr};
// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
std::wstring title{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int xFixedFrame{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int xSize{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int yFixedFrame{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int ySize{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HACCEL acceleratorTable{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
HWND windowHandle{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,hicpp-signed-bitwise)
COLORREF backgroundColor{RGB(255,255,255)};
// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
wil_compat::unique_hbrush backgroundBrush{nullptr};
// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
wil_compat::unique_hpen backgroundPen{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,hicpp-signed-bitwise)
COLORREF clockColor{RGB(255,0,0)};
// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
wil_compat::unique_hbrush clockBrush{nullptr};
// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
wil_compat::unique_hpen clockPen{nullptr};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int cxClient{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
int cyClient{0};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
SYSTEMTIME stPrevious{};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables,clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors)
DirectAnnotation directAnnotation{};

static bool SetNameAndRole()
{
    if (windowHandle == nullptr)
    {
        return false;
    }
    directAnnotation.SetWindow(windowHandle);
    wil_compat::stack_variant self{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    self.vt = VT_I4;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    self.lVal = CHILDID_SELF;
    auto succeeded{directAnnotation.SetAccessibleRole(self, ROLE_SYSTEM_CLOCK)};
    if (!succeeded)
    {
        return false;
    }
    succeeded = directAnnotation.SetAccessibleName(self, L"Analog");
    return succeeded;
}

static bool SetValue()
{
    if (windowHandle == nullptr)
    {
        return false;
    }
    std::array<wchar_t, smallBuffer> timeString{};
    GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &stPrevious, nullptr, timeString.data(),
        gsl::narrow<int>(timeString.size()));
    wil_compat::stack_variant self{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    self.vt = VT_I4;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
    self.lVal = CHILDID_SELF;
    auto succeeded{directAnnotation.SetAccessibleValue(self, timeString.data())};
    VariantClear(&self);
    return succeeded;
}

static void ApplyOptions()
{
    detail::backgroundColor = Options::GetBackgroundColor();
    detail::clockColor = Options::GetClockColor();
    backgroundBrush.reset(CreateSolidBrush(backgroundColor));
    backgroundPen.reset(CreatePen(PS_SOLID, clockPenWidth, backgroundColor));
    clockBrush.reset(CreateSolidBrush(clockColor));
    clockPen.reset(CreatePen(PS_SOLID, clockPenWidth, clockColor));
}

static void SetIsotropic(HDC hdc, int cx, int cy)
{
    SetMapMode(hdc, MM_ISOTROPIC);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    SetWindowExtEx(hdc, 1000, 1000, nullptr);
    SetViewportExtEx(hdc, cx / 2, -cy / 2, nullptr);
    SetViewportOrgEx(hdc, cx / 2, cy / 2, nullptr);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters,clang-diagnostic-unsafe-buffer-usage,cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
static void RotatePoint(gsl::span<POINT> pt, int iAngle)
{
    POINT ptTemp{};
    for (auto& point : pt)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        ptTemp.x = static_cast<LONG>((point.x * std::cos(TWOPI * iAngle / 360)) + (point.y * std::sin(TWOPI * iAngle / 360)));
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        ptTemp.y = static_cast<LONG>((point.y * std::cos(TWOPI * iAngle / 360)) - (point.x * std::sin(TWOPI * iAngle / 360)));
        point = ptTemp;
    }
}

static void DrawClock(HDC hdc)
{
    int iAngle{};
    std::array<POINT, 3> pt{};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    for (iAngle = 0; iAngle < 360; iAngle += 6)
    {
        pt.at(0).x = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        pt.at(0).y = 900;
        RotatePoint(gsl::span<POINT>{pt.data(), 1}, iAngle);
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        pt.at(2).x = pt.at(2).y = (iAngle % 5 != 0) ? 33 : 100;
        pt.at(0).x -= pt.at(2).x / 2;
        pt.at(0).y -= pt.at(2).y / 2;
        pt.at(1).x  = pt.at(0).x + pt.at(2).x;
        pt.at(1).y  = pt.at(0).y + pt.at(2).y;
        SelectObject(hdc, clockBrush.get());
        Ellipse(hdc, pt.at(0).x, pt.at(0).y, pt.at(1).x, pt.at(1).y);
    }
}

static void DrawHands(HDC hdc, const SYSTEMTIME& st, bool fChange)
{
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    static std::array<std::array<POINT, 5>, 3> pt{{
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        {{ { .x = 0, .y = -150 }, { .x = 100, .y = 0 }, { .x = 0, .y = 600 }, { .x = -100, .y = 0 }, { .x = 0, .y = -150 } }},
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        {{ { .x = 0, .y = -200 },  { .x = 50, .y = 0 }, { .x = 0, .y = 800 }, { .x = -50, .y = 0 }, { .x = 0, .y = -200 } }},
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        {{ { .x = 0, .y = 0 }, { .x = 0, .y = 0 }, { .x = 0, .y = 0 }, { .x = 0, .y = 0 }, { .x = 0, .y = 800 } }}
    }};
    size_t i{};
    std::array<int, 3> iAngle{};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    std::array<std::array<POINT, 5>, 3> ptTemp{};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    iAngle[0] = (st.wHour * 30) % 360 + st.wMinute / 2;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    iAngle[1] = st.wMinute * 6;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    iAngle[2] = st.wSecond * 6;
    std::ranges::copy(pt, std::begin(ptTemp));
    for (i = fChange ? 0 : 2 ; i < 3 ; ++i)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        RotatePoint(gsl::span<POINT>{ptTemp.at(i).data(), 5}, iAngle.at(i));
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        Polyline(hdc, ptTemp.at(i).data(), 5);
    }
}

/* Source:
   [How do I switch a window between normal and
   fullscreen?][https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353] */
static void ToggleFullScreen(HWND hWnd)
{
    static WINDOWPLACEMENT prevWindowPlacement{};
    static HMENU prevMenu{nullptr};
    prevWindowPlacement.length = sizeof(prevWindowPlacement);
    const auto windowStyle{GetWindowLongW(hWnd, GWL_STYLE)};
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    if ((windowStyle & WS_OVERLAPPEDWINDOW) != 0)
    {
        MONITORINFO monitorInfo{};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (GetWindowPlacement(hWnd, &prevWindowPlacement) != FALSE
            && GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &monitorInfo) != FALSE)
        {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            SetWindowLongW(hWnd, GWL_STYLE, windowStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(hWnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
            prevMenu = GetMenu(hWnd);
            SetMenu(hWnd, nullptr);
        }
    }
    else
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        SetWindowLongW(hWnd, GWL_STYLE, windowStyle|WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(hWnd, &prevWindowPlacement);
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
        SetMenu(hWnd, prevMenu);
    }
}

static bool ShowContextMenu(HWND hWnd, POINT position)
{
    struct menuItemData
    {
        UINT_PTR menuItemId{};
        std::wstring menuItemName;
    };
    std::vector<menuItemData> menuItems{
        { .menuItemId = IDM_ABOUT, .menuItemName = Utility::loadString(IDS_ABOUT_MENU_ITEM) },
        { .menuItemId = IDM_OPTIONS_EDIT_OPTIONS, .menuItemName = Utility::loadString(IDS_EDIT_OPTIONS_MENU_ITEM) },
        { .menuItemId = IDM_EXIT, .menuItemName = Utility::loadString(IDS_EXIT_MENU_ITEM) }
    };
    if (position.x < 0 && position.y < 0)
    {
        const auto getPos = GetCursorPos(&position);
        if (getPos == FALSE)
        {
            return false;
        }
    }
    wil_compat::unique_hmenu menu(CreatePopupMenu());
    if (!menu)
    {
        return false;
    }
    for (const auto& menuItem : menuItems)
    {
        if (menuItem.menuItemName.empty())
        {
            return false;
        }
        const auto insertMenu = InsertMenuW(menu.get(), static_cast<UINT>(-1), MF_BYPOSITION, menuItem.menuItemId,
            menuItem.menuItemName.c_str());
        if (insertMenu == FALSE)
        {
            return false;
        }
    }
    SetForegroundWindow(hWnd);
    const auto trackPopupMenu{TrackPopupMenu(menu.get(), TPM_BOTTOMALIGN, position.x, position.y, 0, hWnd, nullptr)};
    return (trackPopupMenu != FALSE);
}

static void OnActivate(HWND hWnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    switch (state)
    {
    case WA_ACTIVE:
    case WA_CLICKACTIVE:
    {
        // Do nothing.
    }
    break;
    default:
    {
        // Do nothing.
    }
    break;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise)
    FORWARD_WM_ACTIVATE(hWnd, state, hwndActDeact, fMinimized, DefWindowProcW);
}

static void OnCommand_About(HWND hWnd)
{
    About::Create(detail::hInstance, hWnd, title);
}

static void OnCommand_Exit([[maybe_unused]] HWND hWnd)
{
    DestroyWindow(Get());
}

static void OnCommand_UseDefaultOptions(HWND hWnd)
{
    Options::Reset();
    ApplyOptions();
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
}

static void OnCommand_EditOptions(HWND hWnd)
{
    if (OptionsDialog::Create(detail::hInstance, hWnd) != IDOK)
    {
        return;
    }
    ApplyOptions();
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    RedrawWindow(hWnd, nullptr, nullptr, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
}

static void OnCommand(HWND hWnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDM_ABOUT:
    {
        OnCommand_About(hWnd);
    }
    break;
    case IDM_EXIT:
    {
        OnCommand_Exit(hWnd);
    }
    break;
    case IDM_OPTIONS_USE_DEFAULT:
    {
        OnCommand_UseDefaultOptions(hWnd);
    }
    break;
    case IDM_OPTIONS_EDIT_OPTIONS:
    {
        OnCommand_EditOptions(hWnd);
    }
    break;
    default:
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise)
        FORWARD_WM_COMMAND(hWnd, id, hwndCtl, codeNotify, DefWindowProcW);
    }
    break;
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void OnContextMenu(HWND hWnd,  [[maybe_unused]] HWND hwndContext, UINT xPos, UINT yPos)
{
    POINT position{static_cast<LONG>(xPos), static_cast<LONG>(yPos)};
    ShowContextMenu(hWnd, position);
}

static bool OnCreate(HWND hWnd, [[maybe_unused]] LPCREATESTRUCT lpCreateStruct)
{
    ApplyOptions();
    SetTimer(hWnd, ID_TIMER, TimerElapse, nullptr);
    SYSTEMTIME st{};
    GetLocalTime(&st);
    stPrevious = st;
    return true;
}

static void OnDestroy(HWND hWnd)
{
    KillTimer(hWnd, ID_TIMER);
    windowHandle = nullptr;
    PostQuitMessage(0);
}

static bool OnEraseBackground(HWND hWnd, HDC hdc)
{
    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    SetMapMode(hdc, MM_ANISOTROPIC);
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    SetWindowExtEx(hdc, 100, 100, nullptr);
    SetViewportExtEx(hdc, clientRect.right, clientRect.bottom, nullptr);
    FillRect(hdc, &clientRect, backgroundBrush.get());
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void OnKeyDown(HWND hWnd, UINT vk, [[maybe_unused]] BOOL fDown, int cRepeat, UINT flags)
{
    // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
    switch (vk)
    {
    case VK_F11:
    {
        ToggleFullScreen(hWnd);
    }
    break;
    default:
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        FORWARD_WM_KEYDOWN(hWnd, vk, cRepeat, flags, DefWindowProcW);
    }
    break;
    }
}

static void OnPaint(HWND hWnd)
{
    if (hWnd == nullptr || IsWindow(hWnd) == FALSE)
    {
        return;
    }
    wil_compat::BeginPaint_context paint(hWnd);
    if (!paint.get())
    {
        return;
    }
    SetIsotropic(paint.get(), cxClient, cyClient);
    DrawClock(paint.get());
    DrawHands(paint.get(), stPrevious, true);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void OnSize([[maybe_unused]] HWND hWnd, [[maybe_unused]] UINT state, int cx, int cy)
{
    cxClient = cx;
    cyClient = cy;
}

static void OnSysCommand(HWND hWnd, UINT cmd, int x, int y)
{
    // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
    switch (cmd)
    {
    case IDM_ABOUT:
    {
        OnCommand_About(hWnd);
    }
    break;
    default:
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        FORWARD_WM_SYSCOMMAND(hWnd, cmd, x, y, DefWindowProcW);
    }
    break;
    }
}

static LRESULT OnSystemTrayCallback(HWND hWnd, [[maybe_unused]] WPARAM wParam, LPARAM lParam)
{
    // NOLINTNEXTLINE(bugprone-switch-missing-default-case,clang-diagnostic-switch-default,hicpp-multiway-paths-covered)
    switch (lParam)
    {
    case WM_CONTEXTMENU:
    case WM_RBUTTONUP:
        POINT position{-1, -1};
        return static_cast<LRESULT>(ShowContextMenu(hWnd, position));
    }
    return 0;
}

static void OnTimer(HWND hWnd, [[maybe_unused]] UINT id)
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    const auto timeChanged{st.wHour != stPrevious.wHour || st.wMinute != stPrevious.wMinute};
    wil_compat::GetDC_context dc(hWnd);
    if (!dc.get())
    {
        return;
    }
    SetIsotropic(dc.get(), cxClient, cyClient);
    SelectObject(dc.get(), backgroundPen.get());
    DrawHands(dc.get(), stPrevious, timeChanged);
    SelectObject(dc.get(), clockPen.get());
    DrawHands(dc.get(), st, true);
    stPrevious = st;
    SetValue();
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    HANDLE_MSG(hWnd, WM_ACTIVATE, OnActivate);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    HANDLE_MSG(hWnd, WM_CONTEXTMENU, OnContextMenu);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
    HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    HANDLE_MSG(hWnd, WM_ERASEBKGND, OnEraseBackground);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    HANDLE_MSG(hWnd, WM_KEYDOWN, OnKeyDown);
    HANDLE_MSG(hWnd, WM_PAINT, OnPaint);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    HANDLE_MSG(hWnd, WM_SIZE, OnSize);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    HANDLE_MSG(hWnd, WM_SYSCOMMAND, OnSysCommand);
    HANDLE_MSG(hWnd, WM_TIMER, OnTimer);
    case Utility::to_underlying(appMessages::WM_SYSTEM_TRAY_CALLBACK):
    {
        return OnSystemTrayCallback(hWnd, wParam, lParam);
    }
    default:
    {
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    }
}

} // namespace detail

std::wstring GetWindowClass()
{
    return Utility::loadString(IDC_APP);
}

HWND Get()
{
    return detail::windowHandle;
}

/**
Registers the window class.

@param[in] hInstance
  The handle to the current instance.

@return
  If the function succeeds, the return value is a class atom that uniquely identifies the class
being registered. If the function fails, the return value is zero.
*/
ATOM RegisterWindowClass(HINSTANCE hInstance)
{
    const auto windowClass{GetWindowClass()};
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    wcex.style = CS_HREDRAW|CS_VREDRAW;
    wcex.lpfnWndProc = detail::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_APP);
    wcex.lpszClassName = windowClass.c_str();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    wcex.hIconSm = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_SMALL));
    return RegisterClassExW(&wcex);
}

bool Create(HINSTANCE hInstance, const std::wstring& title)
{
    if (hInstance == nullptr)
    {
        return false;
    }
    if (title.empty())
    {
        return false;
    }
    if (!Options::Load())
    {
        return false;
    }
    detail::hInstance = hInstance;
    detail::title = title;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,performance-no-int-to-ptr)
    detail::acceleratorTable = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_APP_ACCELERATORS));
    if (detail::acceleratorTable == nullptr)
    {
        return false;
    }
    auto classAtom{Window::RegisterWindowClass(hInstance)};
    if (classAtom == 0)
    {
        return false;
    }
    auto windowClass{GetWindowClass()};
    if (windowClass.empty())
    {
        return false;
    }
    detail::xFixedFrame = GetSystemMetrics(SM_CXFIXEDFRAME);
    detail::xSize = GetSystemMetrics(SM_CXSIZE);
    detail::yFixedFrame = GetSystemMetrics(SM_CYFIXEDFRAME);
    detail::ySize = GetSystemMetrics(SM_CYSIZE);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise)
    detail::windowHandle = CreateWindowExW(0, windowClass.c_str(), title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
        CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
    if (detail::windowHandle != nullptr)
    {
        detail::SetNameAndRole();
    }
    return (detail::windowHandle != nullptr);
}

int MessageLoop()
{
    if (detail::windowHandle == nullptr)
    {
        return EXIT_FAILURE;
    }
    BOOL getMessage{FALSE};
    MSG msg {};
    while ((getMessage = GetMessageW(&msg, detail::windowHandle, 0, 0)) != FALSE)
    {
        if (getMessage == -1)
        {
            return -1;
        }
        if (TranslateAcceleratorW(detail::windowHandle, detail::acceleratorTable, &msg) != 0)
        {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (detail::acceleratorTable != nullptr)
    {
        DestroyAcceleratorTable(detail::acceleratorTable);
        detail::acceleratorTable = nullptr;
    }
    return static_cast<int>(msg.wParam);
}

} // namespace Window
