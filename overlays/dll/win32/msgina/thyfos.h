/*
 * PROJECT:     LiberNT Logon GINA DLL
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     THYFOS glass sign-in presentation
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#pragma once

static int ThyfosMin(int A, int B) { return A < B ? A : B; }
static int ThyfosMax(int A, int B) { return A > B ? A : B; }
static float ThyfosMaxFloat(float A, float B) { return A > B ? A : B; }

struct ThyfosLayout
{
    RECT Brand, Panel, Heading, User, Password, Submit, Message, Wave;
};

static ThyfosLayout ThyfosArrange(int Width, int Height, float Scale, int Offset)
{
    ThyfosLayout L = {};
    int P = (int)(460 * Scale), H = (int)(450 * Scale);
    int X = Width - P - (int)(Width * 0.075f) + Offset;
    int Y = (int)(Height * 0.24f);
    int M = (int)(42 * Scale), F = (int)(64 * Scale);
    SetRect(&L.Brand, (int)(Width * 0.075f), (int)(Height * 0.07f),
            Width - (int)(Width * 0.075f), (int)(Height * 0.07f + 170 * Scale));
    SetRect(&L.Panel, X, Y, X + P, Y + H);
    SetRect(&L.Heading, X + M, Y + (int)(61 * Scale), X + P - M, Y + (int)(119 * Scale));
    SetRect(&L.User, X + M, Y + (int)(150 * Scale), X + P - M, Y + (int)(150 * Scale) + F);
    SetRect(&L.Password, X + M, L.User.bottom + (int)(22 * Scale), X + P - M,
            L.User.bottom + (int)(22 * Scale) + F);
    SetRect(&L.Submit, X + M, L.Password.bottom + (int)(28 * Scale), X + P - M,
            L.Password.bottom + (int)(28 * Scale) + F);
    SetRect(&L.Message, X + M, L.Submit.bottom + (int)(8 * Scale), X + P - M, Y + H - (int)(8 * Scale));
    SetRect(&L.Wave, X - (int)(100 * Scale), L.Panel.bottom + (int)(24 * Scale),
            X + P + (int)(16 * Scale), ThyfosMin(Height - (int)(36 * Scale), L.Panel.bottom + (int)(210 * Scale)));
    return L;
}

static void ThyfosRound(Gdiplus::Graphics &G, const RECT &R, float Radius,
                        BYTE Alpha, COLORREF Color, BOOL Border = FALSE)
{
    Gdiplus::GraphicsPath P(Gdiplus::FillModeAlternate);
    float D = Radius * 2.0f;
    P.AddArc((float)R.left, (float)R.top, D, D, 180, 90);
    P.AddArc((float)R.right - D, (float)R.top, D, D, 270, 90);
    P.AddArc((float)R.right - D, (float)R.bottom - D, D, D, 0, 90);
    P.AddArc((float)R.left, (float)R.bottom - D, D, D, 90, 90);
    P.CloseFigure();
    Gdiplus::Color C(Alpha, GetRValue(Color), GetGValue(Color), GetBValue(Color));
    if (Border)
    {
        Gdiplus::Pen Pen(C, 1.5f);
        G.DrawPath(&Pen, &P);
    }
    else
    {
        Gdiplus::SolidBrush Brush(C);
        G.FillPath(&Brush, &P);
    }
}

static void ThyfosText(HDC Dc, HFONT Font, PCWSTR Label, RECT R, COLORREF Color, UINT Align)
{
    HGDIOBJ Old = SelectObject(Dc, Font);
    SetBkMode(Dc, TRANSPARENT);
    SetTextColor(Dc, Color);
    DrawTextW(Dc, Label, -1, &R, Align | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(Dc, Old);
}

static void ThyfosDots(Gdiplus::Graphics &G, const RECT &R, float Scale)
{
    const COLORREF Colors[] = {RGB(255, 110, 119), RGB(255, 204, 103), RGB(132, 218, 109)};
    for (int I = 0; I < 3; ++I)
    {
        Gdiplus::SolidBrush B(Gdiplus::Color(230, GetRValue(Colors[I]), GetGValue(Colors[I]), GetBValue(Colors[I])));
        G.FillEllipse(&B, R.left + (28 + I * 24) * Scale, R.top + 24 * Scale, 14 * Scale, 14 * Scale);
    }
}

static void ThyfosChrome(HDC Dc, Gdiplus::Graphics &G, const ThyfosLayout &L, float Scale,
                         HFONT BrandFont, HFONT HeadingFont, HFONT BodyFont, BOOL Hot, BOOL Busy)
{
    const COLORREF Ink = RGB(21, 26, 32);
    ThyfosText(Dc, BrandFont, L"THYFOS", L.Brand, Ink, DT_LEFT);
    RECT Shadow = L.Panel;
    OffsetRect(&Shadow, 0, (int)(12 * Scale));
    ThyfosRound(G, Shadow, 36 * Scale, 30, RGB(16, 34, 55));
    ThyfosRound(G, L.Panel, 36 * Scale, 217, RGB(235, 243, 250));
    ThyfosRound(G, L.Panel, 36 * Scale, 235, RGB(255, 255, 255), TRUE);
    ThyfosDots(G, L.Panel, Scale);
    ThyfosText(Dc, HeadingFont, L"Log In", L.Heading, Ink, DT_LEFT);
    ThyfosRound(G, L.User, 20 * Scale, 235, RGB(241, 246, 251));
    ThyfosRound(G, L.Password, 20 * Scale, 235, RGB(241, 246, 251));
    ThyfosRound(G, L.User, 20 * Scale, 240, RGB(255, 255, 255), TRUE);
    ThyfosRound(G, L.Password, 20 * Scale, 240, RGB(255, 255, 255), TRUE);
    ThyfosRound(G, L.Submit, 32 * Scale, 255, Hot ? RGB(52, 70, 90) : RGB(46, 58, 73));
    ThyfosRound(G, L.Submit, 32 * Scale, 210, RGB(255, 255, 255), TRUE);
    ThyfosText(Dc, BodyFont, Busy ? L"Signing in..." : L"Log In", L.Submit, RGB(255, 255, 255), DT_CENTER);
    if (L.Wave.bottom > L.Wave.top + 40 * Scale)
    {
        ThyfosRound(G, L.Wave, 36 * Scale, 217, RGB(236, 244, 252));
        ThyfosRound(G, L.Wave, 36 * Scale, 235, RGB(255, 255, 255), TRUE);
        ThyfosDots(G, L.Wave, Scale);
        int Mid = L.Wave.top + (L.Wave.bottom - L.Wave.top) * 3 / 5;
        float Space = (L.Wave.right - L.Wave.left - 40 * Scale) / 41.0f;
        for (int I = 0; I < 41; ++I)
        {
            float T = I / 40.0f;
            float Envelope = expf(-((T - 0.30f) * (T - 0.30f)) / 0.016f) +
                             0.8f * expf(-((T - 0.78f) * (T - 0.78f)) / 0.009f);
            int BarHeight = (int)((L.Wave.bottom - L.Wave.top) * (0.09f + 0.46f * Envelope));
            int X = L.Wave.left + (int)(20 * Scale + I * Space);
            RECT Bar = {X, Mid - BarHeight / 2, X + ThyfosMax(2, (int)(Space * 0.55f)), Mid + BarHeight / 2};
            ThyfosRound(G, Bar, ThyfosMaxFloat(1.0f, Space * 0.25f), 210, RGB(60, 76, 95));
            RECT Reflection = {Bar.left, Bar.bottom + 4, Bar.right, ThyfosMin(L.Wave.bottom - 8, Bar.bottom + BarHeight / 3)};
            if (Reflection.bottom > Reflection.top)
                ThyfosRound(G, Reflection, ThyfosMaxFloat(1.0f, Space * 0.25f), 32, RGB(77, 132, 180));
        }
    }
}
