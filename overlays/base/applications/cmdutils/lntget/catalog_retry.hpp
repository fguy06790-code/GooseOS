/*
 * PROJECT:     THYFOS Store
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Bound retries to transient network failures
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#include <windows.h>
#include <wininet.h>

namespace lntget
{
inline bool CatalogRetryable(DWORD code)
{
    return code == ERROR_NETWORK_UNREACHABLE || code == ERROR_NOT_CONNECTED ||
        code == ERROR_INTERNET_TIMEOUT || code == ERROR_INTERNET_NAME_NOT_RESOLVED ||
        code == ERROR_INTERNET_CANNOT_CONNECT || code == ERROR_INTERNET_CONNECTION_ABORTED ||
        code == ERROR_INTERNET_CONNECTION_RESET;
}
}
