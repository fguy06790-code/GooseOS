/*
 * PROJECT:     THYFOS Setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Prepare the bundled Wi-Fi driver before network selection
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */

#include "libernt.h"
#include <wlanapi.h>
#include "oobe.h"
#include "resource.h"

#define PM_NETWORK_READY (WM_APP + 8)

static BOOL NetworkBusy;
static BOOL NetworkReady;
static DWORD NetworkResult;

DWORD NetworkSetupResult(VOID)
{
    return NetworkReady ? NetworkResult : ERROR_NOT_READY;
}

static BOOL NetworkHasWifi(VOID)
{
    HANDLE Client;
    DWORD Version;
    PWLAN_INTERFACE_INFO_LIST List = NULL;
    BOOL Present = FALSE;
    if (WlanOpenHandle(2, NULL, &Version, &Client) != ERROR_SUCCESS)
        return FALSE;
    if (WlanEnumInterfaces(Client, NULL, &List) == ERROR_SUCCESS && List)
        Present = List->dwNumberOfItems != 0;
    if (List) WlanFreeMemory(List);
    WlanCloseHandle(Client, NULL);
    return Present;
}

static VOID NetworkOpenWifi(HWND Page)
{
    WCHAR File[MAX_PATH], Command[MAX_PATH + 64];
    STARTUPINFOW Startup = { sizeof(Startup) };
    PROCESS_INFORMATION Process;
    DWORD Error;
    if (!GetWindowsDirectoryW(File, ARRAYSIZE(File)) ||
        FAILED(StringCchCatW(File, ARRAYSIZE(File), L"\\ImmersiveControlPanel\\SystemSettings.exe")) ||
        FAILED(StringCchPrintfW(Command, ARRAYSIZE(Command), L"\"%s\" ms-settings:network-wifi", File)))
        return;
    if (CreateProcessW(File, Command, NULL, NULL, FALSE, 0, NULL, NULL, &Startup, &Process))
    {
        CloseHandle(Process.hThread);
        CloseHandle(Process.hProcess);
        return;
    }
    Error = GetLastError();
    StringCchPrintfW(Command, ARRAYSIZE(Command), L"Wi-Fi Settings could not open (error %lu). You can continue offline and try Settings later.", Error);
    SetupMessage(GetParent(Page), TD_ERROR_ICON, TDCBF_OK_BUTTON, IDOK, L"Wi-Fi setup", Command);
}

static VOID NetworkShowState(HWND Page, BOOL OpenWifi)
{
    WCHAR Text[512];
    DWORD Reboot = 0, UsbReboot = 0, Bytes = sizeof(Reboot);
    BOOL Present = NetworkHasWifi();
    RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\THYFOS\\Setup\\IntelWiFi24.80", L"RebootRequired",
        RRF_RT_REG_DWORD, NULL, &Reboot, &Bytes);
    Bytes = sizeof(UsbReboot);
    RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\THYFOS\\Setup\\AsusUsbWifi", L"RebootRequired",
        RRF_RT_REG_DWORD, NULL, &UsbReboot, &Bytes);
    Reboot = Reboot || UsbReboot;
    ShowWindow(GetDlgItem(Page, IDC_PROCESSPROGRESS), SW_HIDE);
    SetDlgItemTextW(Page, IDC_ACTIVITY, L"Choose your connection");
    if (NetworkResult != ERROR_SUCCESS)
        StringCchPrintfW(Text, ARRAYSIZE(Text),
            L"Wi-Fi driver setup did not complete (error %lu). Check Windows\\DriverSetup for the package and log. You can use an existing adapter, Ethernet, or continue offline.", NetworkResult);
    else if (Reboot && !Present)
        StringCchCopyW(Text, ARRAYSIZE(Text),
            L"Wi-Fi driver setup requested a restart. Finish setup offline, then restart to activate the driver. Ethernet can be used now. Wi-Fi Settings will be available after restart if the adapter starts successfully.");
    else if (Present)
        StringCchCopyW(Text, ARRAYSIZE(Text),
            L"Wi-Fi is available. Choose your network in Wi-Fi Settings, select Connect, and enter its password. Return here and choose Next when you are ready.");
    else
        StringCchCopyW(Text, ARRAYSIZE(Text),
            L"No usable Wi-Fi adapter is available. If this is VirtualBox, connect Windows to Wi-Fi; the VM uses virtual Ethernet. Otherwise check the driver or use Ethernet. You can continue offline.");
    SetDlgItemTextW(Page, IDC_ITEM, Text);
    EnableDlgItem(Page, IDC_NETWORK_WIFI, TRUE);
    EnableDlgItem(Page, IDC_NETWORK_REFRESH, TRUE);
    if (Present && OpenWifi) NetworkOpenWifi(Page);
}

static DWORD WINAPI NetworkDriverThread(LPVOID Parameter)
{
    HWND Page = (HWND)Parameter;
    OOBE_PROGRESS Progress;
    DWORD Result;
    DWORD UsbResult;
    DWORD ServiceResult;
    Progress.hwndActivity = GetDlgItem(Page, IDC_ACTIVITY);
    Progress.hwndItem = GetDlgItem(Page, IDC_ITEM);
    Progress.hwndProgress = GetDlgItem(Page, IDC_PROCESSPROGRESS);
    Result = InstallBundledWifi(&Progress);
    UsbResult = InstallBundledUsbWifi(&Progress);
    if (Result == ERROR_SUCCESS) Result = UsbResult;
    ServiceResult = EnsureWifiService();
    if (Result == ERROR_SUCCESS) Result = ServiceResult;
    PostMessageW(Page, PM_NETWORK_READY, Result, 0);
    return Result;
}

INT_PTR CALLBACK NetworkPageProc(HWND Page, UINT Message, WPARAM WParam, LPARAM LParam)
{
    HANDLE Thread;
    UNREFERENCED_PARAMETER(LParam);
    switch (Message)
    {
        case SPN_SETACTIVE:
            if (NetworkReady)
            {
                SetupFrameSetButtons(Page, TRUE, TRUE);
                NetworkShowState(Page, FALSE);
                return TRUE;
            }
            if (NetworkBusy) return TRUE;
            NetworkBusy = TRUE;
            SetupFrameSetButtons(Page, FALSE, FALSE);
            EnableSetupCancel(GetParent(Page), FALSE);
            EnableDlgItem(Page, IDC_NETWORK_WIFI, FALSE);
            EnableDlgItem(Page, IDC_NETWORK_REFRESH, FALSE);
            SetDlgItemTextW(Page, IDC_ACTIVITY, L"Preparing your Wi-Fi driver");
            SetDlgItemTextW(Page, IDC_ITEM, L"Checking Intel wireless and ASUS USB Wi-Fi hardware against the bundled offline drivers. Network selection follows this step.");
            Thread = CreateThread(NULL, 0, NetworkDriverThread, Page, 0, NULL);
            if (Thread) CloseHandle(Thread);
            else PostMessageW(Page, PM_NETWORK_READY, GetLastError(), 0);
            return TRUE;
        case PM_NETWORK_READY:
            NetworkResult = (DWORD)WParam;
            NetworkReady = TRUE;
            NetworkBusy = FALSE;
            EnableSetupCancel(GetParent(Page), TRUE);
            SetupFrameSetButtons(Page, TRUE, TRUE);
            NetworkShowState(Page, TRUE);
            return TRUE;
        case WM_COMMAND:
            if (NetworkBusy) return TRUE;
            if (LOWORD(WParam) == IDC_NETWORK_WIFI) { NetworkOpenWifi(Page); return TRUE; }
            if (LOWORD(WParam) == IDC_NETWORK_REFRESH)
            {
                NetworkReady = FALSE;
                SendMessageW(Page, SPN_SETACTIVE, 0, 0);
                return TRUE;
            }
            break;
        case SPN_WIZNEXT:
        case SPN_WIZBACK:
            if (NetworkBusy) { SetWindowLongPtrW(Page, DWLP_MSGRESULT, -1); return TRUE; }
            break;
    }
    return FALSE;
}
