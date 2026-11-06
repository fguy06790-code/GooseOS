/*
 * PROJECT:     THYFOS Store
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Bound retries to transient network failures
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#include <windows.h>
#include <wininet.h>
#include <thyfos_goose.h>

namespace lntget
{
inline bool CatalogRetryable(DWORD code)
{
    return thyfos_catalog_retryable(code) != 0;
}
}
