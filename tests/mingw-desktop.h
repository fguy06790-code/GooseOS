/*
 * PROJECT:     THYFOS Desktop tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Preserve Windows SDK macros across MinGW C++ headers
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <strsafe.h>
#include <algorithm>

#undef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(value) ((void)(value))
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
