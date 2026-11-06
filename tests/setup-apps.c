/*
 * PROJECT:     THYFOS Setup tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify bounded component downloads and complete-file hashing
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#define CINTERFACE
#define COBJMACROS
#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <strsafe.h>
#include <initguid.h>
#include <objbase.h>
#include <bits.h>
#include <bcrypt.h>
#include <stdio.h>
#include <wchar.h>
#include "oobe.h"

static DWORD Tick;
static int Mode, Polls, Cancels, Completes, Releases, Reads, HashFinishes;
#define StateFailure HRESULT_FROM_WIN32(ERROR_CONNECTION_ABORTED)
static HANDLE ProcessHeap;
static WCHAR FixturePath[MAX_PATH];

static HINF WINAPI MockOpenInf(PCWSTR, PCWSTR Class, DWORD Style, PUINT ErrorLine)
{ return SetupOpenInfFileW(FixturePath, Class, Style, ErrorLine); }
static BOOL CreateFixture()
{
    WCHAR Folder[MAX_PATH];
    if (!GetTempPathW(ARRAYSIZE(Folder), Folder) ||
        !GetTempFileNameW(Folder, L"app", 0, FixturePath)) return FALSE;
    HANDLE File = CreateFileW(FixturePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (File == INVALID_HANDLE_VALUE) return FALSE;
    static const char Inf[] =
        "[Version]\r\nSignature=\"$Windows NT$\"\r\n"
        "[Apps.amd64]\r\nPresent\r\nUnsigned\r\nValid\r\n"
        "[Present]\r\nName=Present\r\nUrl=https://example.invalid/a\r\n"
        "File=a.exe\r\nType=exe\r\nSha256=stale-hash\r\nSpace=999\r\nUnattended=yes\r\n"
        "Detect=\"%11%\\kernel32.dll\"\r\n"
        "[Unsigned]\r\nName=Unsigned\r\nUrl=https://example.invalid/b\r\nFile=b.exe\r\nType=exe\r\n"
        "[Valid]\r\nName=Valid\r\nUrl=https://example.invalid/c\r\nFile=c.exe\r\nType=exe\r\nSigner=Publisher\r\n";
    DWORD Written;
    BOOL Ok = WriteFile(File, Inf, sizeof(Inf) - 1, &Written, NULL) && Written == sizeof(Inf) - 1;
    CloseHandle(File);
    return Ok;
}

VOID LoadPrintfW(PWSTR, SIZE_T, UINT, ...) {}
VOID SetupFramePressNext(HWND) {}
INT SetupMessage(HWND, PCWSTR, TASKDIALOG_COMMON_BUTTON_FLAGS, INT, PCWSTR, PCWSTR) { return IDOK; }
static HRESULT WINAPI MockCoCreate(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID *Object)
{ *Object = (PVOID)1; return S_OK; }
static DWORD WINAPI MockTick() { return Tick; }
static VOID WINAPI MockSleep(DWORD Delay) { Tick += Delay; }
static HRESULT MockCreateJob(IBackgroundCopyManager*, LPCWSTR, BG_JOB_TYPE, GUID*, IBackgroundCopyJob **Job)
{ *Job = (IBackgroundCopyJob*)2; return S_OK; }
static HRESULT MockAddFile(IBackgroundCopyJob*, LPCWSTR, LPCWSTR)
{ return Mode == 3 ? E_ACCESSDENIED : S_OK; }
static HRESULT MockState(IBackgroundCopyJob*, BG_JOB_STATE *State)
{
    ++Polls;
    if (Mode == 2 || Polls > 1000) return StateFailure;
    *State = Mode == 5 ? BG_JOB_STATE_CANCELLED : Mode == 1 ? BG_JOB_STATE_TRANSFERRED :
        ((Polls & 1) ? BG_JOB_STATE_TRANSIENT_ERROR : BG_JOB_STATE_QUEUED);
    return S_OK;
}
static HRESULT MockProgress(IBackgroundCopyJob*, BG_JOB_PROGRESS *Progress)
{ ZeroMemory(Progress, sizeof(*Progress)); return S_OK; }
static HRESULT MockCancel(IBackgroundCopyJob*) { ++Cancels; return S_OK; }
static HRESULT MockComplete(IBackgroundCopyJob*) { ++Completes; return S_OK; }
static ULONG MockRelease(PVOID) { ++Releases; return 0; }
static HANDLE WINAPI MockCreateFile(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE)
{ return (HANDLE)3; }
static BOOL WINAPI MockRead(HANDLE, LPVOID Buffer, DWORD, LPDWORD Count, LPOVERLAPPED)
{
    ++Reads;
    if (Reads == 1) { memcpy(Buffer, "abc", 3); *Count = 3; return TRUE; }
    *Count = 0;
    if (Mode == 4) { SetLastError(ERROR_READ_FAULT); return FALSE; }
    return TRUE;
}
static BOOL WINAPI MockClose(HANDLE) { return TRUE; }
static NTSTATUS WINAPI MockFinish(BCRYPT_HASH_HANDLE Hash, PUCHAR Output, ULONG Size, ULONG Flags)
{ ++HashFinishes; return BCryptFinishHash(Hash, Output, Size, Flags); }

#undef IBackgroundCopyManager_CreateJob
#undef IBackgroundCopyManager_Release
#undef IBackgroundCopyJob_AddFile
#undef IBackgroundCopyJob_SetMinimumRetryDelay
#undef IBackgroundCopyJob_SetNoProgressTimeout
#undef IBackgroundCopyJob_Resume
#undef IBackgroundCopyJob_GetState
#undef IBackgroundCopyJob_GetProgress
#undef IBackgroundCopyJob_GetError
#undef IBackgroundCopyJob_Cancel
#undef IBackgroundCopyJob_Complete
#undef IBackgroundCopyJob_Release
#define IBackgroundCopyManager_CreateJob MockCreateJob
#define IBackgroundCopyManager_Release MockRelease
#define IBackgroundCopyJob_AddFile MockAddFile
#define IBackgroundCopyJob_SetMinimumRetryDelay(Job, Delay) S_OK
#define IBackgroundCopyJob_SetNoProgressTimeout(Job, Delay) S_OK
#define IBackgroundCopyJob_Resume(Job) S_OK
#define IBackgroundCopyJob_GetState MockState
#define IBackgroundCopyJob_GetProgress MockProgress
#define IBackgroundCopyJob_GetError(Job, Error) E_NOTIMPL
#define IBackgroundCopyJob_Cancel MockCancel
#define IBackgroundCopyJob_Complete MockComplete
#define IBackgroundCopyJob_Release MockRelease
#define SetupOpenInfFileW MockOpenInf
#define CoCreateInstance MockCoCreate
#define GetTickCount MockTick
#define Sleep MockSleep
#define CreateFileW MockCreateFile
#define ReadFile MockRead
#define CloseHandle MockClose
#define BCryptFinishHash MockFinish
#define __analysis_noreturn
#define _LIBERNT_PCH_
#define PROCESSOR_ARCHITECTURE_RISCV64 15
#define SPN_SETACTIVE (WM_APP + 0x100)
#define SPN_WIZNEXT (WM_APP + 0x102)
#include "../overlays/base/setup/libernt/apps.c"

static void Reset(int CurrentMode)
{
    Mode = CurrentMode;
    Tick = 0;
    Polls = Cancels = Completes = Releases = Reads = HashFinishes = 0;
}

int main()
{
    ProcessHeap = GetProcessHeap();
    OOBE_APP App = {0};
    OOBE_PROGRESS Progress = {0};
    Reset(0);
    if (DownloadFile(&App, L"unused", &Progress) != HRESULT_FROM_WIN32(ERROR_TIMEOUT) ||
        Tick != APP_NO_PROGRESS_TIMEOUT || Cancels != 1 || Releases != 2) return 1;
    Reset(0); Tick = 0xffff0000;
    if (DownloadFile(&App, L"unused", &Progress) != HRESULT_FROM_WIN32(ERROR_TIMEOUT) ||
        Cancels != 1) return 2;
    Reset(1);
    if (FAILED(DownloadFile(&App, L"unused", &Progress)) || Cancels || Completes != 1 || Releases != 2) return 3;
    Reset(2);
    if (DownloadFile(&App, L"unused", &Progress) != StateFailure || Cancels != 1 || Releases != 2) return 4;
    Reset(3);
    if (DownloadFile(&App, L"unused", &Progress) != E_ACCESSDENIED || Cancels != 1 || Releases != 2) return 5;
    Reset(5);
    if (DownloadFile(&App, L"unused", &Progress) != HRESULT_FROM_WIN32(ERROR_CANCELLED) || Cancels || Releases != 2) return 10;
    static const WCHAR Hash[] = L"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    Reset(0);
    if (!FileHashMatches(L"unused", Hash) || Reads != 2 || HashFinishes != 1) return 6;
    Reset(4);
    if (FileHashMatches(L"unused", Hash) || Reads != 2 || HashFinishes) return 7;
    if (!CreateFixture()) return 8;
    POOBE_APP Apps = NULL;
    ULONG Count = LoadOobeApps(&Apps, FALSE);
    DeleteFileW(FixturePath);
    if (Count != 1 || wcscmp(Apps[0].Name, L"Valid") || Apps[0].Sha256[0] ||
        Apps[0].SpaceMb || Apps[0].Unattended || wcscmp(Apps[0].Signer, L"Publisher")) return 9;
    HeapFree(ProcessHeap, 0, Apps);
    puts("Component download timeouts, cancellation, error cleanup, complete-file hashing, and isolated INF entries passed");
    return 0;
}
