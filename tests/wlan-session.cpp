/*
 * PROJECT:     LiberNT Settings tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Preserve WLAN operation failures and close session resources
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include <windows.h>
#include <wlanapi.h>
#include <stdio.h>

static DWORD OpenResult, EnumResult, QueryResult, SetResult, QuerySize;
static int Closes, Frees, Sets;
static WLAN_INTERFACE_INFO_LIST Interfaces;
static WLAN_RADIO_STATE Radio;

static DWORD WINAPI MockOpen(DWORD, PVOID, PDWORD Version, PHANDLE Client)
{ *Version = 2; *Client = (HANDLE)1; return OpenResult; }
static DWORD WINAPI MockEnum(HANDLE, PVOID, PWLAN_INTERFACE_INFO_LIST *List)
{ *List = &Interfaces; return EnumResult; }
static DWORD WINAPI MockQuery(HANDLE, const GUID *, WLAN_INTF_OPCODE, PVOID, PDWORD Size, PVOID *Data,
                              PWLAN_OPCODE_VALUE_TYPE)
{ *Size = QuerySize; *Data = QueryResult ? NULL : &Radio; return QueryResult; }
static DWORD WINAPI MockSet(HANDLE, const GUID *, WLAN_INTF_OPCODE, DWORD, const VOID *, PVOID)
{ ++Sets; return SetResult; }
static VOID WINAPI MockFree(PVOID) { ++Frees; }
static DWORD WINAPI MockClose(HANDLE, PVOID) { ++Closes; return ERROR_SUCCESS; }

#define WlanOpenHandle MockOpen
#define WlanEnumInterfaces MockEnum
#define WlanQueryInterface MockQuery
#define WlanSetInterface MockSet
#define WlanFreeMemory MockFree
#define WlanCloseHandle MockClose
#include "wlan-session.h"

static void Reset()
{
    OpenResult = EnumResult = QueryResult = SetResult = ERROR_SUCCESS;
    QuerySize = sizeof(Radio); Closes = Frees = Sets = 0;
    ZeroMemory(&Interfaces, sizeof(Interfaces)); Interfaces.dwNumberOfItems = 1;
    ZeroMemory(&Radio, sizeof(Radio)); Radio.dwNumberOfPhys = 1;
    Radio.PhyRadioState[0].dot11SoftwareRadioState = dot11_radio_state_on;
    Radio.PhyRadioState[0].dot11HardwareRadioState = dot11_radio_state_on;
}

int main()
{
    Reset();
    {
        WlanSession wlan;
        if (!wlan.Available() || wlan.Result() || !wlan.RadioOn() || wlan.Result()) return 1;
        if (wlan.SetRadio(false) || Sets != 1) return 2;
    }
    if (Closes != 1 || Frees != 2) return 3;
    Reset(); OpenResult = ERROR_SERVICE_NOT_ACTIVE;
    {
        WlanSession wlan;
        if (wlan.Available() || wlan.Result() != ERROR_SERVICE_NOT_ACTIVE ||
            wlan.SetRadio(true) != ERROR_SERVICE_NOT_ACTIVE || Sets) return 4;
    }
    if (Closes || Frees) return 5;
    Reset(); EnumResult = ERROR_ACCESS_DENIED;
    {
        WlanSession wlan;
        if (wlan.Available() || wlan.Result() != ERROR_ACCESS_DENIED) return 6;
    }
    if (Closes != 1 || Frees) return 7;
    Reset(); QueryResult = ERROR_NOT_READY;
    {
        WlanSession wlan;
        if (wlan.RadioOn() || wlan.Result() != ERROR_NOT_READY) return 8;
    }
    if (Closes != 1 || Frees != 1) return 9;
    Reset(); Radio.PhyRadioState[0].dot11SoftwareRadioState = dot11_radio_state_off;
    {
        WlanSession wlan;
        if (wlan.RadioOn() || wlan.Result() != ERROR_SUCCESS) return 10;
    }
    Reset(); SetResult = ERROR_ACCESS_DENIED;
    {
        WlanSession wlan;
        if (wlan.SetRadio(true) != ERROR_ACCESS_DENIED || wlan.Result() != ERROR_ACCESS_DENIED) return 11;
    }
    Reset(); QuerySize = 0;
    {
        WlanSession wlan;
        if (wlan.RadioOn() || wlan.Result() != ERROR_INVALID_DATA) return 12;
    }
    if (Closes != 1 || Frees != 2) return 13;
    Reset(); Interfaces.dwNumberOfItems = 0;
    {
        WlanSession wlan;
        if (wlan.Available() || wlan.Result() || wlan.SetRadio(true) != ERROR_NOT_FOUND || Sets) return 14;
    }
    puts("WLAN service, enumeration, radio query/set failures and resource cleanup passed without hardware changes");
    return 0;
}
