/*
 * PROJECT:     THYFOS Setup tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify WLAN service start and bounded failure recovery
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include <windows.h>
#include <winsvc.h>
#include <stdio.h>
static DWORD State, StartError, ExitCode, QueryError;
static int Starts, Queries, Sleeps, Closes;
static int CompleteAfter;
static BOOL StopBeforeStart, FailAfterStart;
static SC_HANDLE WINAPI MockManager(LPCWSTR, LPCWSTR, DWORD) { return (SC_HANDLE)1; }
static SC_HANDLE WINAPI MockService(SC_HANDLE, LPCWSTR, DWORD) { return (SC_HANDLE)2; }
static BOOL WINAPI MockClose(SC_HANDLE) { ++Closes; return TRUE; }
static BOOL WINAPI MockQuery(SC_HANDLE, SC_STATUS_TYPE, LPBYTE Buffer, DWORD, LPDWORD Size)
{
    ++Queries; *Size = sizeof(SERVICE_STATUS_PROCESS);
    if (QueryError) { SetLastError(QueryError); return FALSE; }
    SERVICE_STATUS_PROCESS *Status = (SERVICE_STATUS_PROCESS*)Buffer;
    ZeroMemory(Status, sizeof(*Status));
    if (State == SERVICE_STOP_PENDING && StopBeforeStart && Sleeps >= 3) State = SERVICE_STOPPED;
    if (State == SERVICE_START_PENDING && Sleeps >= CompleteAfter && CompleteAfter >= 0)
        State = FailAfterStart ? SERVICE_STOPPED : SERVICE_RUNNING;
    Status->dwCurrentState = State;
    Status->dwWin32ExitCode = ExitCode;
    return TRUE;
}
static BOOL WINAPI MockStart(SC_HANDLE, DWORD, LPCWSTR*)
{
    ++Starts;
    if (StartError)
    {
        if (StartError == ERROR_SERVICE_ALREADY_RUNNING) State = SERVICE_START_PENDING;
        SetLastError(StartError); return FALSE;
    }
    if (State != SERVICE_STOPPED) { SetLastError(ERROR_SERVICE_CANNOT_ACCEPT_CTRL); return FALSE; }
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
{
    State = Current; StartError = ExitCode = QueryError = 0;
    Starts = Queries = Sleeps = Closes = 0; CompleteAfter = 0;
    StopBeforeStart = FailAfterStart = FALSE;
}
int main()
{
    Reset(SERVICE_START_PENDING); CompleteAfter = 3;
    if (ThyfosStartWifiService() || Starts || Sleeps != 3 || Closes != 2) return 5;
    Reset(SERVICE_RUNNING);
    if (ThyfosStartWifiService() || Starts || Closes != 2) return 1;
    Reset(SERVICE_STOPPED);
    if (ThyfosStartWifiService() || Starts != 1 || Closes != 2) return 2;
    Reset(SERVICE_STOPPED); StartError = ERROR_SERVICE_DISABLED;
    if (ThyfosStartWifiService() != ERROR_SERVICE_DISABLED || Closes != 2) return 3;
    Reset(SERVICE_STOPPED); CompleteAfter = -1;
    if (ThyfosStartWifiService() != ERROR_TIMEOUT || Sleeps != 300 || Starts != 1 || Closes != 2) return 4;
    Reset(SERVICE_STOP_PENDING); StopBeforeStart = TRUE; CompleteAfter = 4;
    if (ThyfosStartWifiService() || Starts != 1 || Sleeps != 4 || Closes != 2) return 6;
    Reset(SERVICE_STOPPED); CompleteAfter = 80;
    if (ThyfosStartWifiService() || Starts != 1 || Sleeps != 80 || Closes != 2) return 7;
    Reset(SERVICE_STOPPED); FailAfterStart = TRUE; ExitCode = ERROR_SERVICE_DEPENDENCY_FAIL;
    if (ThyfosStartWifiService() != ERROR_SERVICE_DEPENDENCY_FAIL || Starts != 1 || Closes != 2) return 8;
    Reset(SERVICE_RUNNING); QueryError = ERROR_ACCESS_DENIED;
    if (ThyfosStartWifiService() != ERROR_ACCESS_DENIED || Starts || Closes != 2) return 9;
    Reset(SERVICE_PAUSED);
    if (ThyfosStartWifiService() != ERROR_SERVICE_CANNOT_ACCEPT_CTRL || Starts || Closes != 2) return 10;
    Reset(SERVICE_START_PENDING); CompleteAfter = 3; FailAfterStart = TRUE; ExitCode = ERROR_SERVICE_DEPENDENCY_FAIL;
    if (ThyfosStartWifiService() != ERROR_SERVICE_DEPENDENCY_FAIL || Starts || Closes != 2) return 11;
    Reset(SERVICE_STOPPED); StartError = ERROR_SERVICE_ALREADY_RUNNING; CompleteAfter = 3;
    if (ThyfosStartWifiService() || Starts != 1 || Sleeps != 3 || Closes != 2) return 12;
    puts("WLAN startup, pending transitions, delayed readiness and bounded failures passed without host service changes");
    return 0;
}
