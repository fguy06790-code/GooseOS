/*
 * PROJECT:     THYFOS Setup
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     Identify Intel PCI wireless adapters before offline setup
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#pragma once
#include <wchar.h>

static BOOL ThyfosIntelWifiIds(const WCHAR *Hardware, const WCHAR *Compatible)
{
    BOOL Intel=FALSE, Wireless=FALSE;
    const WCHAR *Id;
    for (Id=Hardware; *Id; Id+=wcslen(Id)+1)
        if (!_wcsnicmp(Id,L"PCI\\VEN_8086&",13)) Intel=TRUE;
    for (Id=Compatible; *Id; Id+=wcslen(Id)+1)
        if (!_wcsnicmp(Id,L"PCI\\CC_0280",11)) Wireless=TRUE;
    return Intel && Wireless;
}
