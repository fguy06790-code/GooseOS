/*
 * PROJECT:     THYFOS Desktop tests
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Isolate launcher and window-policy tests from audio and icon rendering
 * COPYRIGHT:   Copyright 2026 THYFOS contributors
 */
#include <shellframe.h>
#include "widgets.h"

void DesktopMeters::SampleResources() {}
void DesktopMeters::SampleAudio() {}
void DesktopMeters::StopAudio() {}

void ShellFrame::FillIcon(Gdiplus::Graphics &, LPCWSTR, const Gdiplus::RectF &, COLORREF) {}
