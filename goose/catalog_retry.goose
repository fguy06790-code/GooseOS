// PROJECT: THYFOS Store Goose port
// LICENSE: GPL-3.0-or-later
// Keep retries limited to transient network failures. Values are the Win32
// winerror.h and WinINet wininet.h constants used by CatalogRetryable.
namespace catalog;

let ERROR_NETWORK_UNREACHABLE: u32 = 1231;
let ERROR_NOT_CONNECTED: u32 = 2250;
let ERROR_INTERNET_TIMEOUT: u32 = 12002;
let ERROR_INTERNET_NAME_NOT_RESOLVED: u32 = 12007;
let ERROR_INTERNET_CANNOT_CONNECT: u32 = 12029;
let ERROR_INTERNET_CONNECTION_ABORTED: u32 = 12030;
let ERROR_INTERNET_CONNECTION_RESET: u32 = 12031;

export fn thyfos_catalog_retryable(code: u32) -> bool {
    return code == ERROR_NETWORK_UNREACHABLE ||
           code == ERROR_NOT_CONNECTED ||
           code == ERROR_INTERNET_TIMEOUT ||
           code == ERROR_INTERNET_NAME_NOT_RESOLVED ||
           code == ERROR_INTERNET_CANNOT_CONNECT ||
           code == ERROR_INTERNET_CONNECTION_ABORTED ||
           code == ERROR_INTERNET_CONNECTION_RESET;
}
