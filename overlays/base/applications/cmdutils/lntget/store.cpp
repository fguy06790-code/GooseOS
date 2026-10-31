/*
 * PROJECT:     THYFOS Store
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Graphical entry point for the shared package catalog
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */

#include "gui.hpp"
#include "util.hpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR commandLine, int)
{
    lntget::SourceManager source;
    const lntget::Status status = lntget::RunCatalogGui(source, lntget::Utf8FromWide(commandLine));
    if (!status)
    {
        MessageBoxW(nullptr, lntget::WideFromUtf8(status.message).c_str(), L"THYFOS Store", MB_OK | MB_ICONERROR);
        return 1;
    }
    return 0;
}
