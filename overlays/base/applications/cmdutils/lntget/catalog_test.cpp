/*
 * PROJECT:     THYFOS Store tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify the catalog UI clears an offline error after recovery
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include <windows.h>
static HANDLE WINAPI MockThread(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD)
{
    return CreateEventW(nullptr, TRUE, TRUE, nullptr);
}
#define CreateThread MockThread
#undef NOMINMAX
#include "gui.cpp"
#undef CreateThread
#include <cstdio>

int wmain()
{
    goose_init(0, nullptr);
    using namespace lntget;
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES};
    InitCommonControlsEx(&controls);
    CatalogWindow state;
    state.theme.Create();
    HINSTANCE instance = GetModuleHandleW(nullptr);
    state.window = CreateWindowW(L"STATIC", L"Store recovery test", WS_POPUP, 0, 0, 1040, 700, nullptr, nullptr, instance, nullptr);
    if (!state.window) return 1;
    SetWindowLongPtrW(state.window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));
    state.info.theme = &state.theme;
    state.info.window = CreateWindowW(L"STATIC", L"", WS_CHILD, 0, 0, 600, 200, state.window, nullptr, instance, nullptr);
    state.list = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD | LVS_REPORT | LVS_OWNERDATA, 0, 0, 600, 300, state.window, nullptr, instance, nullptr);
    state.tree = CreateWindowW(WC_TREEVIEWW, L"", WS_CHILD, 0, 0, 200, 300, state.window, nullptr, instance, nullptr);
    state.refresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD, 0, 0, 100, 30, state.window, nullptr, instance, nullptr);
    state.install = CreateWindowW(L"BUTTON", L"Install", WS_CHILD, 0, 0, 100, 30, state.window, nullptr, instance, nullptr);
    state.download = CreateWindowW(L"BUTTON", L"Download", WS_CHILD, 0, 0, 100, 30, state.window, nullptr, instance, nullptr);
    state.operationTitle = CreateWindowW(L"STATIC", L"", WS_CHILD, 0, 0, 300, 30, state.window, nullptr, instance, nullptr);
    state.operationDetail = CreateWindowW(L"STATIC", L"", WS_CHILD, 0, 0, 600, 30, state.window, nullptr, instance, nullptr);
    if (!state.info.window || !state.list || !state.tree || !state.refresh || !state.install || !state.download ||
        !state.operationTitle || !state.operationDetail || !state.worker.Start(state.window, nullptr)) return 2;
    LVCOLUMNW column{};
    column.mask = LVCF_WIDTH;
    column.cx = 200;
    ListView_InsertColumn(state.list, 0, &column);
    ListView_InsertColumn(state.list, 1, &column);
    CatalogResult *offline = new CatalogResult;
    offline->error = "DNS unavailable";
    offline->code = ERROR_INTERNET_NAME_NOT_RESOLVED;
    CatalogProc(state.window, MessageCatalogReady, 0, reinterpret_cast<LPARAM>(offline));
    if (state.failure.empty() || state.info.blocks.empty() || state.catalogRetries != 1) return 3;
    WCHAR text[32]; GetWindowTextW(state.refresh, text, 32);
    if (wcscmp(text, L"Retry")) return 4;
    CatalogResult *ready = new CatalogResult;
    ready->ok = true;
    ready->catalog.packages.emplace_back();
    ready->catalog.packages[0].name = L"Example";
    ready->catalog.packages[0].utf8Id = "Example.Tool";
    ready->catalog.counts.resize(CategoryCount());
    CatalogProc(state.window, MessageCatalogReady, 0, reinterpret_cast<LPARAM>(ready));
    if (!state.failure.empty() || !state.info.blocks.empty() || state.catalogRetries || state.filtered.size() != 1) return 5;
    GetWindowTextW(state.refresh, text, 32);
    if (wcscmp(text, L"Refresh")) return 6;
    state.selected = 0;
    state.loading = true;
    BeginOperation(state, true);
    if (state.operationBusy) return 7;
    state.loading = false;
    state.operationBusy = true;
    CatalogProc(state.window, WM_COMMAND, IdRefresh, 0);
    if (state.loading) return 8;
    state.failure = "DNS unavailable";
    CatalogProc(state.window, WM_TIMER, TimerCatalogRetry, 0);
    if (state.loading) return 9;
    state.failure.clear();
    state.operationBusy = false;
    NMITEMACTIVATE activation{};
    activation.hdr.idFrom = IdList;
    activation.hdr.code = NM_DBLCLK;
    activation.iItem = -1;
    activation.ptAction = {500, 250};
    HandleNotify(state, reinterpret_cast<LPARAM>(&activation));
    if (state.operationBusy) return 10;
    RECT subitem{};
    if (!ListView_GetSubItemRect(state.list, 0, 1, LVIR_BOUNDS, &subitem)) return 18;
    activation.ptAction = {(subitem.left + subitem.right) / 2, (subitem.top + subitem.bottom) / 2};
    HandleNotify(state, reinterpret_cast<LPARAM>(&activation));
    if (!state.operationBusy) return 19;
    state.operationBusy = false;
    state.info.blocks.emplace_back();
    NMLISTVIEW deselected{};
    deselected.hdr.idFrom = IdList;
    deselected.hdr.code = LVN_ITEMCHANGED;
    deselected.iItem = 0;
    deselected.uChanged = LVIF_STATE;
    deselected.uOldState = LVIS_SELECTED;
    HandleNotify(state, reinterpret_cast<LPARAM>(&deselected));
    if (state.selected != -1 || !state.info.blocks.empty() || IsWindowEnabled(state.install) || IsWindowEnabled(state.download)) return 11;
    state.selected = 0;
    deselected.iItem = -1;
    HandleNotify(state, reinterpret_cast<LPARAM>(&deselected));
    if (state.selected != -1) return 17;
    BeginOperation(state, true);
    if (state.operationBusy) return 12;
    OperationUpdate completed;
    completed.stage = OperationStage::Complete;
    completed.install = true;
    completed.packageName = L"Example";
    completed.exitCode = ERROR_SUCCESS_REBOOT_REQUIRED;
    ApplyOperationUpdate(state, completed);
    GetWindowTextW(state.operationTitle, text, _countof(text));
    if (wcscmp(text, L"Restart required")) return 13;
    WCHAR detail[256];
    GetWindowTextW(state.operationDetail, detail, _countof(detail));
    if (!wcsstr(detail, L"Restart your computer")) return 14;
    completed.exitCode = ERROR_SUCCESS_REBOOT_INITIATED;
    ApplyOperationUpdate(state, completed);
    GetWindowTextW(state.operationTitle, text, _countof(text));
    if (wcscmp(text, L"Restart initiated")) return 15;
    completed.exitCode = ERROR_SUCCESS;
    ApplyOperationUpdate(state, completed);
    GetWindowTextW(state.operationDetail, detail, _countof(detail));
    if (wcsstr(detail, L"ready to use") || !wcsstr(detail, L"installer completed")) return 16;
    state.worker.Stop();
    DestroyWindow(state.window);
    state.theme.Destroy();
    std::puts("Catalog recovery, operation exclusion, selection clearing, empty-background activation and restart results passed; no network or installer launched");
    return 0;
}
