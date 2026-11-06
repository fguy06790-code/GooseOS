/*
 * PROJECT:     THYFOS Setup tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Keep installer packages unavailable in component regression tests
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#pragma once
#define THYFOS_USB_WIFI_FILE_COUNT 0
#define THYFOS_USB_WIFI_INF L"unused-test-driver.inf"
#define THYFOS_USB_WIFI_HARDWARE_ID L"unused-test-hardware"
static const struct { const WCHAR *Name; const WCHAR *Hash; } ThyfosUsbWifiFiles[] = {{L"", L""}};
