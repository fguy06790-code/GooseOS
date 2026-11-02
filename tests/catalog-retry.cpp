/*
 * PROJECT:     THYFOS Store tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Verify network retries cannot retry trust or format failures
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include "catalog_retry.hpp"
#include <stdio.h>
int main()
{
    const DWORD transient[] = {ERROR_NETWORK_UNREACHABLE, ERROR_NOT_CONNECTED, ERROR_INTERNET_TIMEOUT,
        ERROR_INTERNET_NAME_NOT_RESOLVED, ERROR_INTERNET_CANNOT_CONNECT, ERROR_INTERNET_CONNECTION_ABORTED,
        ERROR_INTERNET_CONNECTION_RESET};
    for (DWORD code : transient) if (!lntget::CatalogRetryable(code)) return 1;
    const DWORD permanent[] = {ERROR_SUCCESS, ERROR_ACCESS_DENIED, ERROR_BAD_FORMAT, ERROR_CRC,
        ERROR_INTERNET_SEC_CERT_DATE_INVALID, ERROR_INTERNET_INVALID_CA, ERROR_INVALID_DATA};
    for (DWORD code : permanent) if (lntget::CatalogRetryable(code)) return 2;
    puts("Transient network retries accepted; certificate, format and access errors excluded");
    return 0;
}
