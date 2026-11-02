/*
 * PROJECT:     THYFOS Setup tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify WLAN service start and bounded failure recovery
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
static DWORD State, StartError;
static int Starts, Queries, Sleeps, Closes;
static BOOL Complete;
static SC_HANDLE WINAPI MockManager(LPCWSTR, LPCWSTR, DWORD) { return (SC_HANDLE)1; }
static SC_HANDLE WINAPI MockService(SC_HANDLE, LPCWSTR, DWORD) { return (SC_HANDLE)2; }
static BOOL WINAPI MockClose(SC_HANDLE) { ++Closes; return TRUE; }
static BOOL WINAPI MockQuery(SC_HANDLE, SC_STATUS_TYPE, LPBYTE Buffer, DWORD, LPDWORD Size)
{
    ++Queries; *Size = sizeof(SERVICE_STATUS_PROCESS);
    SERVICE_STATUS_PROCESS *Status = (SERVICE_STATUS_PROCESS*)Buffer;
    ZeroMemory(Status, sizeof(*Status));
    if (Starts && Complete) State = SERVICE_RUNNING;
    Status->dwCurrentState = State;
    return TRUE;
}
static BOOL WINAPI MockStart(SC_HANDLE, DWORD, LPCWSTR*)
{
    ++Starts;
    if (StartError) { SetLastError(StartError); return FALSE; }
    State = SERVICE_START_PENDING; return TRUE;
}
static VOID WINAPI MockSleep(DWORD) { ++Sleeps; }
#define OpenSCManagerW MockManager
#define OpenServiceW MockService
#define CloseServiceHandle MockClose
#define QueryServiceStatusEx MockQuery
#define StartServiceW MockStart
#define Sleep MockSleep
#include "wifi-service.h"
static void Reset(DWORD Current)
{ State = Current; StartError = 0; Starts = Queries = Sleeps = Closes = 0; Complete = TRUE; }
int main()
{
    Reset(SERVICE_RUNNING);
    if (ThyfosStartWifiService() || Starts || Closes != 2) return 1;
    Reset(SERVICE_STOPPED);
    if (ThyfosStartWifiService() || Starts != 1 || Closes != 2) return 2;
    Reset(SERVICE_STOPPED); StartError = ERROR_SERVICE_DISABLED;
    if (ThyfosStartWifiService() != ERROR_SERVICE_DISABLED || Closes != 2) return 3;
    Reset(SERVICE_STOPPED); Complete = FALSE;
    if (ThyfosStartWifiService() != ERROR_TIMEOUT || Sleeps != 50 || Closes != 2) return 4;
    puts("WLAN service running/start/disabled/timeout handling passed without host service changes");
    return 0;
}
