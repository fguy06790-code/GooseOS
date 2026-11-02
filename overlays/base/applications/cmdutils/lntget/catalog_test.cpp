/*
 * PROJECT:     THYFOS Store tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify the catalog UI clears an offline error after recovery
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include "gui.cpp"
#include <cstdio>

int wmain()
{
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
    if (!state.info.window || !state.list || !state.tree || !state.refresh) return 2;
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
    DestroyWindow(state.window);
    state.theme.Destroy();
    std::puts("Offline error, retry label and stale error pane clear after catalog recovery; no network or installer launched");
    return 0;
}
