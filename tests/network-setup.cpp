/*
 * PROJECT:     THYFOS Setup tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify driver preparation precedes Wi-Fi Settings without host installation
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */

#include <windows.h>
#include <commctrl.h>
#include <wlanapi.h>
#include <strsafe.h>
#include <stdio.h>
#include "oobe.h"

static int Installs, Opens;
static BOOL Next, Cancel, Adapter, ThreadFailure;
static DWORD DriverResult, PostedResult, ServiceResult;
static LPTHREAD_START_ROUTINE PendingThread;
static LPVOID PendingParameter;
static WCHAR Executable[MAX_PATH], Arguments[MAX_PATH + 64];
static WLAN_INTERFACE_INFO_LIST Interfaces;

VOID SetupFrameSetButtons(HWND, BOOL, BOOL Value) { Next = Value; }
VOID EnableSetupCancel(HWND, BOOL Value) { Cancel = Value; }
INT SetupMessage(HWND, PCWSTR, TASKDIALOG_COMMON_BUTTON_FLAGS, INT, PCWSTR, PCWSTR) { return IDOK; }
DWORD InstallBundledWifi(POOBE_PROGRESS) { ++Installs; return DriverResult; }
DWORD InstallBundledUsbWifi(POOBE_PROGRESS) { return ERROR_SUCCESS; }
DWORD EnsureWifiService(VOID) { return ServiceResult; }
static HANDLE WINAPI MockThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE Entry, LPVOID Parameter, DWORD, LPDWORD)
{
    if (ThreadFailure) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
    PendingThread = Entry; PendingParameter = Parameter;
    return CreateEventW(NULL, FALSE, FALSE, NULL);
}
static BOOL WINAPI MockPost(HWND, UINT, WPARAM Result, LPARAM) { PostedResult = (DWORD)Result; return TRUE; }
static DWORD WINAPI MockOpen(DWORD, PVOID, PDWORD Version, PHANDLE Client)
{ *Version = 2; *Client = (HANDLE)1; return ERROR_SUCCESS; }
static DWORD WINAPI MockEnum(HANDLE, PVOID, PWLAN_INTERFACE_INFO_LIST *List)
{ Interfaces.dwNumberOfItems = Adapter ? 1 : 0; *List = &Interfaces; return ERROR_SUCCESS; }
static VOID WINAPI MockFree(PVOID) {}
static DWORD WINAPI MockClose(HANDLE, PVOID) { return ERROR_SUCCESS; }
static BOOL WINAPI MockProcess(LPCWSTR File, LPWSTR Command, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES,
    BOOL, DWORD, LPVOID, LPCWSTR, LPSTARTUPINFOW, LPPROCESS_INFORMATION Process)
{
    ++Opens;
    StringCchCopyW(Executable, _countof(Executable), File);
    StringCchCopyW(Arguments, _countof(Arguments), Command);
    Process->hProcess = CreateEventW(NULL, FALSE, FALSE, NULL);
    Process->hThread = CreateEventW(NULL, FALSE, FALSE, NULL);
    return TRUE;
}

#define _LIBERNT_PCH_
#define SPN_SETACTIVE (WM_APP + 0x100)
#define SPN_WIZNEXT (WM_APP + 0x102)
#define SPN_WIZBACK (WM_APP + 0x103)
#define EnableDlgItem(Page, Id, Value) EnableWindow(GetDlgItem(Page, Id), Value)
#define CreateThread MockThread
#define PostMessageW MockPost
#define WlanOpenHandle MockOpen
#define WlanEnumInterfaces MockEnum
#define WlanFreeMemory MockFree
#define WlanCloseHandle MockClose
#define CreateProcessW MockProcess
#define SendMessageW MockSend
static LRESULT WINAPI MockSend(HWND Page, UINT Message, WPARAM WParam, LPARAM LParam)
{ return NetworkPageProc(Page, Message, WParam, LParam); }
#include "network.c"

static void Reset()
{
    Installs = Opens = 0; Next = Cancel = TRUE; Adapter = TRUE; ThreadFailure = FALSE;
    DriverResult = PostedResult = ServiceResult = 0; NetworkReady = NetworkBusy = FALSE;
    PendingThread = NULL;
}

int wmain()
{
    Reset();
    if (NetworkSetupResult() != ERROR_NOT_READY) return 1;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0);
    if (Next || Cancel || Opens || Installs || !PendingThread) return 2;
    NetworkPageProc(NULL, WM_COMMAND, IDC_NETWORK_WIFI, 0);
    if (Opens) return 3;
    PendingThread(PendingParameter);
    if (Installs != 1 || Opens) return 4;
    NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (!Next || !Cancel || Opens != 1 || NetworkSetupResult()) return 5;
    WCHAR Expected[MAX_PATH]; GetWindowsDirectoryW(Expected, _countof(Expected));
    StringCchCatW(Expected, _countof(Expected), L"\\ImmersiveControlPanel\\SystemSettings.exe");
    if (lstrcmpiW(Executable, Expected) || !wcsstr(Arguments, L"ms-settings:network-wifi")) return 6;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0);
    if (Installs != 1 || Opens != 1) return 7;
    Reset(); Adapter = FALSE;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0); PendingThread(PendingParameter);
    NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (Opens || !Next || !Cancel) return 8;
    Adapter = TRUE; NetworkPageProc(NULL, WM_COMMAND, IDC_NETWORK_REFRESH, 0);
    if (Next || Cancel) return 9;
    PendingThread(PendingParameter); NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (Opens != 1 || Installs != 2) return 9;
    Reset(); Adapter = FALSE; DriverResult = ERROR_TIMEOUT;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0); PendingThread(PendingParameter);
    NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (NetworkSetupResult() != ERROR_TIMEOUT || !Next || !Cancel || Opens) return 10;
    Reset(); ThreadFailure = TRUE;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0);
    NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (NetworkSetupResult() != ERROR_NOT_ENOUGH_MEMORY || !Next || !Cancel || Installs) return 11;
    Reset(); Adapter = FALSE; ServiceResult = ERROR_SERVICE_DISABLED;
    NetworkPageProc(NULL, SPN_SETACTIVE, 0, 0); PendingThread(PendingParameter);
    NetworkPageProc(NULL, PM_NETWORK_READY, PostedResult, 0);
    if (NetworkSetupResult() != ERROR_SERVICE_DISABLED || !Next || !Cancel || Opens) return 12;
    puts("Driver-before-Wi-Fi ordering, installed Settings path, offline continuation, revisit, refresh and failure recovery passed; no host installer or Wi-Fi change");
    return 0;
}
