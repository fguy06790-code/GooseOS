/*
 * PROJECT:     THYFOS Setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Start WLAN AutoConfig during first-boot setup
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#include <winsvc.h>

static DWORD ThyfosStartWifiService(VOID)
{
    SC_HANDLE Manager, Service;
    SERVICE_STATUS_PROCESS Status;
    DWORD Bytes, Result, Attempt;
    BOOL Started = FALSE;
    Manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!Manager) return GetLastError();
    Service = OpenServiceW(Manager, L"WlanSvc", SERVICE_START | SERVICE_QUERY_STATUS);
    Result = Service ? ERROR_SUCCESS : GetLastError();
    CloseServiceHandle(Manager);
    if (!Service) return Result;
    Result = ERROR_TIMEOUT;
    for (Attempt = 0; Attempt <= 300; ++Attempt)
    {
        if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (BYTE*)&Status, sizeof(Status), &Bytes))
        { Result = GetLastError(); break; }
        if (Status.dwCurrentState == SERVICE_RUNNING)
        { Result = ERROR_SUCCESS; break; }
        if (Status.dwCurrentState == SERVICE_STOPPED)
        {
            if (Started)
            {
                Result = Status.dwWin32ExitCode ? Status.dwWin32ExitCode : ERROR_SERVICE_NOT_ACTIVE;
                break;
            }
            if (Attempt == 300) break;
            if (!StartServiceW(Service, 0, NULL))
            {
                DWORD Error = GetLastError();
                if (Error != ERROR_SERVICE_ALREADY_RUNNING)
                { Result = Error; break; }
            }
            Started = TRUE;
        }
        else if (Status.dwCurrentState == SERVICE_START_PENDING)
            Started = TRUE;
        else if (Status.dwCurrentState != SERVICE_STOP_PENDING)
        { Result = ERROR_SERVICE_CANNOT_ACCEPT_CTRL; break; }
        if (Attempt != 300) Sleep(100);
    }
    CloseServiceHandle(Service);
    return Result;
}
