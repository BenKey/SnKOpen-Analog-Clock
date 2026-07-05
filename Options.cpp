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

#include "mingw_wil_compat.h"

#include "Options.h"

#include "resource.h"

namespace Options {

namespace detail {

const auto optionsKeyName{LR"(SOFTWARE\Benilda Key\SnKOpen Analog Clock)"};

const auto backgroundColorValueName{L"BackgroundColor"};
const auto clockColorValueName{L"ClockColor"};

// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF defaultBackgroundColor{RGB(255,255,255)};
// NOLINTNEXTLINE(hicpp-signed-bitwise)
const COLORREF defaultClockColor{RGB(255,0,0)};

// NOLINTNEXTLINE(clang-diagnostic-exit-time-destructors,clang-diagnostic-global-constructors,cppcoreguidelines-avoid-non-const-global-variables)
wil_compat::unique_hkey optionsKey{};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
COLORREF backgroundColor{defaultBackgroundColor};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
COLORREF clockColor{defaultClockColor};

static bool InitOptionsKey()
{
    DWORD disposition{};
    HKEY rawKey{};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast,hicpp-signed-bitwise,performance-no-int-to-ptr)
    const auto result{RegCreateKeyExW(HKEY_CURRENT_USER, optionsKeyName, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, nullptr,
        &rawKey, &disposition)};
    if (result == ERROR_SUCCESS)
    {
        optionsKey.reset(rawKey);
    }
    return (result == ERROR_SUCCESS);
}

static DWORD QueryValue(LPCWSTR valueName, DWORD defaultValue)
{
    if (!optionsKey)
    {
        return defaultValue;
    }
    ULONG valueSizeInBytes = sizeof(DWORD);
    DWORD valueType = 0;
    DWORD value = 0;
    const auto result = RegQueryValueExW(optionsKey.get(), valueName, nullptr, &valueType, reinterpret_cast<LPBYTE>(&value),
        &valueSizeInBytes);
    if (result != ERROR_SUCCESS)
    {
        return defaultValue;
    }
    if (valueType != REG_DWORD)
    {
        return defaultValue;
    }
    return value;
}

static bool SetValue(LPCWSTR valueName, DWORD value)
{
    if (!optionsKey)
    {
        return false;
    }
    const auto result = RegSetValueExW(optionsKey.get(), valueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(DWORD));
    return (result == ERROR_SUCCESS);
}

} // namespace detail

bool Load()
{
    if (!detail::InitOptionsKey())
    {
        return false;
    }
    detail::backgroundColor = detail::QueryValue(detail::backgroundColorValueName, detail::defaultBackgroundColor);
    detail::clockColor = detail::QueryValue(detail::clockColorValueName, detail::defaultClockColor);
    return true;
}

bool Reset()
{
	detail::backgroundColor = detail::defaultBackgroundColor;
	detail::clockColor = detail::defaultClockColor;
    return Save();
}

bool Save()
{
    const auto ret = (detail::SetValue(detail::backgroundColorValueName, detail::backgroundColor)
        && detail::SetValue(detail::clockColorValueName, detail::clockColor));
    return ret;
}

COLORREF GetBackgroundColor()
{
    return detail::backgroundColor;
}

COLORREF GetClockColor()
{
    return detail::clockColor;
}

void SetBackgroundColor(COLORREF color)
{
    detail::backgroundColor = color;
}

void SetClockColor(COLORREF color)
{
    detail::clockColor = color;
}

} // namespace Options
