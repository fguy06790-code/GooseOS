#include <windows.h>
#include <shellapi.h>
static WCHAR CapturedPath[1024];
static int LaunchCount;
static HINSTANCE WINAPI MockShellExecute(HWND,LPCWSTR,LPCWSTR File,LPCWSTR,LPCWSTR,INT)
{
    lstrcpynW(CapturedPath,File,_countof(CapturedPath)); ++LaunchCount;
    return (HINSTANCE)33;
}
#define ShellExecuteW MockShellExecute
#define wWinMain ThyfosApplicationMain
#include "desktop.cpp"
#undef wWinMain
#undef ShellExecuteW
#include <stdio.h>
int wmain()
{
    WCHAR Expected[MAX_PATH];
    GetWindowsDirectoryW(Expected,_countof(Expected));
    StringCchCatW(Expected,_countof(Expected),L"\\ImmersiveControlPanel\\SystemSettings.exe");
    OpenSettings();
    if (LaunchCount!=1 || lstrcmpiW(CapturedPath,Expected)) return 1;
    OpenLauncherApp(2);
    if (LaunchCount!=2 || lstrcmpiW(CapturedPath,Expected)) return 2;
    Search=CreateWindowExW(0,L"EDIT",L"settings",WS_POPUP,0,0,200,30,NULL,NULL,GetModuleHandleW(NULL),NULL);
    if (!Search) return 3;
    OpenCommand();
    if (LaunchCount!=3 || lstrcmpiW(CapturedPath,Expected)) return 4;
    SetWindowTextW(Search,L"systemsettings.exe"); OpenCommand();
    if (LaunchCount!=4 || lstrcmpiW(CapturedPath,Expected)) return 5;
    DestroyWindow(Search); Search=NULL;
    puts("Settings action, launcher row and aliases resolve the installed Settings path; no app launched");
    return 0;
}
