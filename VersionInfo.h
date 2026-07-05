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

#ifndef VersionInfo_h_03214944
#define VersionInfo_h_03214944

#define VER_START_YEAR 2022
#define VER_START_YEAR_STRING "2022"

#define VER_APP_NAME_STR "SnKOpen Analog Clock"
#define DESCRIPTION "SnKOpen Analog Clock, based on the Clock program from the book Programming Windows by Charles Petzold."
#define VER_FILE_DESCRIPTION_STR DESCRIPTION
#define VER_INTERNAL_NAME_STR "Clock.exe"
#define VER_ORIGINAL_FILE_NAME_STR "Clock.exe"
#define VER_PRODUCT_NAME_STR VER_APP_NAME_STR

#include "VersionInfoCommon.h"

#ifdef DESCRIPTION
#  define DESCRIPTION_W WIDEN(DESCRIPTION)
#endif

#endif
