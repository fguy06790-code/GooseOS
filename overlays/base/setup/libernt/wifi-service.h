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
    Manager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!Manager) return GetLastError();
    Service = OpenServiceW(Manager, L"WlanSvc", SERVICE_START | SERVICE_QUERY_STATUS);
    Result = Service ? ERROR_SUCCESS : GetLastError();
    CloseServiceHandle(Manager);
    if (!Service) return Result;
    if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (BYTE*)&Status, sizeof(Status), &Bytes))
        Result = GetLastError();
    else if (Status.dwCurrentState != SERVICE_RUNNING)
    {
        if (!StartServiceW(Service, 0, NULL) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
            Result = GetLastError();
        if (Result == ERROR_SUCCESS)
        {
            Result = ERROR_TIMEOUT;
            for (Attempt = 0; Attempt < 50; ++Attempt)
            {
                if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO, (BYTE*)&Status, sizeof(Status), &Bytes))
                { Result = GetLastError(); break; }
                if (Status.dwCurrentState == SERVICE_RUNNING) { Result = ERROR_SUCCESS; break; }
                if (Status.dwCurrentState == SERVICE_STOPPED)
                { Result = Status.dwWin32ExitCode ? Status.dwWin32ExitCode : ERROR_SERVICE_NOT_ACTIVE; break; }
                Sleep(100);
            }
        }
    }
    CloseServiceHandle(Service);
    return Result;
}
