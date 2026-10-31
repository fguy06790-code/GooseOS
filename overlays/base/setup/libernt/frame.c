/*
 * PROJECT:     LiberNT Setup
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Window shared by Setup and the first-boot setup
 * COPYRIGHT:   Copyright 2018-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Copyright 2026 Ahmed ARIF <arif.ing@outlook.com>
 */

#include "libernt.h"

#include "resource.h"
#include <reactos/dwmsettings.h>
#include "../../shell/thyfos/preferences.h"

#define NDEBUG
#include <debug.h>

typedef struct _SETUP_FRAME
{
    PSETUPDATA SetupData;
    HWND hwnd;
    const SETUP_PAGE_INFO* PageInfo;
    INT PageCount;
    HWND Pages[SETUP_MAX_PAGES];
    INT Current;
    INT Restart;
    HBITMAP hWatermark;
    SIZE Watermark;
    INT Corner;
    HBRUSH Background;
    HBRUSH Sidebar;
    HFONT BodyFont;
} SETUP_FRAME;

static SETUP_FRAME Frame;

static VOID FrameApplyGlass(HWND hwnd)
{
    DWORD Opacity=85,Bytes=sizeof(Opacity);
    HIGHCONTRASTW Contrast={sizeof(Contrast)};
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(Contrast),&Contrast,0) &&
        (Contrast.dwFlags&HCF_HIGHCONTRASTON))
    { DwmSettingsClearContentBackdrop(hwnd); return; }
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_WINDOW_OPACITY,RRF_RT_REG_DWORD,NULL,&Opacity,&Bytes);
    if (Opacity<30 || Opacity>100) Opacity=85;
    DwmSettingsSetContentBackdrop(hwnd,RGB(239,245,252),(Opacity*255+50)/100);
    SetPropW(hwnd,DWM_PROP_BACKDROP_COLORIZATION,(HANDLE)(ULONG_PTR)(RGB(221,234,248)+1));
}

static BOOL FrameMotion(VOID)
{
    BOOL Motion = TRUE;
    ANIMATIONINFO Info = {sizeof(Info)};
    if (SystemParametersInfoW(SPI_GETANIMATION, sizeof(Info), &Info, 0) && !Info.iMinAnimate) return FALSE;
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &Motion, 0);
    return Motion;
}

static LRESULT CALLBACK FramePageProc(HWND hwnd, UINT Message, WPARAM WParam, LPARAM LParam,
                                     UINT_PTR Id, DWORD_PTR Data)
{
    UNREFERENCED_PARAMETER(Id);
    UNREFERENCED_PARAMETER(Data);
    if (Message == WM_CTLCOLORDLG || Message == WM_CTLCOLORSTATIC || Message == WM_CTLCOLORBTN)
    {
        SetTextColor((HDC)WParam, RGB(29, 49, 73));
        SetBkColor((HDC)WParam, RGB(239, 245, 252));
        return (LRESULT)Frame.Background;
    }
    if (Message == WM_NCDESTROY) RemoveWindowSubclass(hwnd, FramePageProc, 1);
    return DefSubclassProc(hwnd, Message, WParam, LParam);
}

static BOOL CALLBACK FrameSetFont(HWND hwnd, LPARAM Font)
{
    WCHAR Class[32];
    SendMessageW(hwnd, WM_SETFONT, Font, FALSE);
    GetClassNameW(hwnd,Class,ARRAYSIZE(Class));
    if (!lstrcmpiW(Class,L"Edit") || !lstrcmpiW(Class,L"ComboBox") ||
        (!lstrcmpiW(Class,L"Button") && (GetWindowLongPtrW(hwnd,GWL_STYLE)&BS_TYPEMASK)<=BS_DEFPUSHBUTTON))
    {
        RECT Rect;
        GetWindowRect(hwnd,&Rect);
        HRGN Region=CreateRoundRectRgn(0,0,Rect.right-Rect.left+1,Rect.bottom-Rect.top+1,10,10);
        if (Region && !SetWindowRgn(hwnd,Region,TRUE)) DeleteObject(Region);
    }
    return TRUE;
}

static VOID FrameDrawButton(const DRAWITEMSTRUCT *Item)
{
    RECT Rect = Item->rcItem;
    BOOL Primary = Item->CtlID == IDC_FRAME_NEXT;
    BOOL Enabled = !(Item->itemState & ODS_DISABLED);
    COLORREF Color = Primary ? RGB(43, 111, 172) : RGB(229, 238, 248);
    HBRUSH Brush;
    HPEN Pen;
    HGDIOBJ OldBrush, OldPen, OldFont;
    WCHAR Text[128];
    if (Item->itemState & ODS_SELECTED) Color = Primary ? RGB(28, 80, 133) : RGB(207, 222, 240);
    if (!Enabled) Color = RGB(221, 229, 239);
    Brush = CreateSolidBrush(Color); Pen = CreatePen(PS_SOLID, 1, Primary ? Color : RGB(186, 206, 226));
    OldBrush = SelectObject(Item->hDC, Brush); OldPen = SelectObject(Item->hDC, Pen);
    RoundRect(Item->hDC, Rect.left, Rect.top, Rect.right, Rect.bottom, 14, 14);
    SelectObject(Item->hDC, OldBrush); SelectObject(Item->hDC, OldPen);
    DeleteObject(Brush); DeleteObject(Pen);
    OldFont = SelectObject(Item->hDC, Frame.BodyFont);
    SetBkMode(Item->hDC, TRANSPARENT);
    SetTextColor(Item->hDC, !Enabled ? RGB(126, 143, 163) : Primary ? RGB(255, 255, 255) : RGB(29, 49, 73));
    GetWindowTextW(Item->hwndItem, Text, ARRAYSIZE(Text));
    DrawTextW(Item->hDC, Text, -1, &Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (Item->itemState & ODS_FOCUS) { InflateRect(&Rect, -4, -4); DrawFocusRect(Item->hDC, &Rect); }
    SelectObject(Item->hDC, OldFont);
}

VOID
LoadPrintfW(
    _Out_writes_(cchText) PWSTR Text,
    _In_ SIZE_T cchText,
    _In_ UINT uID,
    ...)
{
    WCHAR Format[512];
    va_list Args;

    Text[0] = UNICODE_NULL;
    if (!LoadStringW(GetModuleHandleW(NULL), uID, Format, ARRAYSIZE(Format)))
        return;

    va_start(Args, uID);
    StringCchVPrintfW(Text, cchText, Format, Args);
    va_end(Args);
}

INT
SetupMessage(
    _In_opt_ HWND hwndOwner,
    _In_opt_ PCWSTR Icon,
    _In_ TASKDIALOG_COMMON_BUTTON_FLAGS Buttons,
    _In_ INT DefaultButton,
    _In_opt_ PCWSTR MainInstruction,
    _In_opt_ PCWSTR Content)
{
    TASKDIALOGCONFIG Config;
    INT Button = IDCANCEL;

    ZeroMemory(&Config, sizeof(Config));
    Config.cbSize = sizeof(Config);
    Config.hwndParent = hwndOwner;
    Config.hInstance = GetModuleHandleW(NULL);
    Config.dwFlags = hwndOwner ? TDF_POSITION_RELATIVE_TO_WINDOW : 0;
    Config.dwCommonButtons = Buttons;
    Config.pszWindowTitle = MAKEINTRESOURCEW(IDS_CAPTION);
    Config.pszMainIcon = Icon;
    Config.pszMainInstruction = MainInstruction;
    Config.pszContent = Content;
    Config.nDefaultButton = DefaultButton;

    if (FAILED(TaskDialogIndirect(&Config, &Button, NULL, NULL)))
        return IDCANCEL;
    return Button;
}

static HFONT
CreateFrameFont(
    _In_ INT PointSize)
{
    NONCLIENTMETRICSW Metrics;
    LOGFONTW Font;
    HDC hdc;

    Metrics.cbSize = sizeof(Metrics);
    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(Metrics), &Metrics, 0))
        return NULL;

    Font = Metrics.lfMessageFont;
    Font.lfWeight = FW_BOLD;
    if (PointSize)
    {
        hdc = GetDC(NULL);
        Font.lfHeight = -MulDiv(PointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC(NULL, hdc);
    }
    return CreateFontIndirectW(&Font);
}

VOID
SetResText(
    _In_ HWND hWnd,
    _In_ UINT uID)
{
    WCHAR Text[512];

    Text[0] = UNICODE_NULL;
    if (uID)
        LoadStringW(Frame.SetupData->hInstance, uID, Text, ARRAYSIZE(Text));
    SetWindowTextW(hWnd, Text);
}

VOID
EnableSetupCancel(
    _In_ HWND hFrame,
    _In_ BOOL Enable)
{
    EnableDlgItem(hFrame, IDCANCEL, Enable);
    EnableMenuItem(GetSystemMenu(hFrame, FALSE),
                   SC_CLOSE,
                   MF_BYCOMMAND | (Enable ? MF_ENABLED : MF_GRAYED));
}

VOID
SetupFrameSetButtons(
    _In_ HWND hPage,
    _In_ BOOL Back,
    _In_ BOOL Next)
{
    HWND hFrame = GetParent(hPage);

    EnableDlgItem(hFrame, IDC_FRAME_BACK, Back);
    EnableDlgItem(hFrame, IDC_FRAME_NEXT, Next);
}

VOID
SetupFrameShowButtons(
    _In_ HWND hPage,
    _In_ BOOL Back,
    _In_ BOOL Next,
    _In_ BOOL Cancel)
{
    HWND hFrame = GetParent(hPage);

    ShowDlgItem(hFrame, IDC_FRAME_BACK, Back ? SW_SHOW : SW_HIDE);
    ShowDlgItem(hFrame, IDC_FRAME_NEXT, Next ? SW_SHOW : SW_HIDE);
    ShowDlgItem(hFrame, IDCANCEL, Cancel ? SW_SHOW : SW_HIDE);
}

VOID
SetupFrameSetButtonText(
    _In_ HWND hPage,
    _In_ INT ButtonId,
    _In_ UINT uID)
{
    SetResText(GetDlgItem(GetParent(hPage), ButtonId), uID);
}

VOID
SetupFrameSetTitle(
    _In_ HWND hPage,
    _In_ UINT Title,
    _In_ UINT Subtitle)
{
    HWND hFrame = GetParent(hPage);

    SetResText(GetDlgItem(hFrame, IDC_FRAME_TITLE), Title);
    SetResText(GetDlgItem(hFrame, IDC_FRAME_SUBTITLE), Subtitle);
}

VOID
SetupFramePressNext(
    _In_ HWND hPage)
{
    HWND hFrame = GetParent(hPage);

    PostMessageW(hFrame, WM_COMMAND, MAKEWPARAM(IDC_FRAME_NEXT, BN_CLICKED),
                 (LPARAM)GetDlgItem(hFrame, IDC_FRAME_NEXT));
}

static VOID
FrameShowPage(
    _In_ INT Page)
{
    HWND hPage = Frame.Pages[Page];
    BOOL Back = !!(Frame.PageInfo[Page].Flags & SETUP_PAGE_BACK);
    BOOL Cancel = !(Frame.PageInfo[Page].Flags & SETUP_PAGE_NO_CANCEL);

    if (Frame.Current >= 0)
    {
        SendMessageW(Frame.Pages[Frame.Current], SPN_KILLACTIVE, 0, 0);
        ShowWindow(Frame.Pages[Frame.Current], SW_HIDE);
    }

    Frame.Current = Page;
    SetupFrameSetTitle(hPage, Frame.PageInfo[Page].Title, Frame.PageInfo[Page].Subtitle);
    SetupFrameSetButtonText(hPage, IDC_FRAME_BACK, IDS_BACKBTN);
    SetupFrameSetButtonText(hPage, IDC_FRAME_NEXT, IDS_NEXTBTN);
    SetupFrameSetButtonText(hPage, IDCANCEL, IDS_CANCELBTN);
    SetupFrameShowButtons(hPage, Back, TRUE, Cancel);
    SetupFrameSetButtons(hPage, Back, TRUE);
    EnableSetupCancel(Frame.hwnd, Cancel);

    if (!FrameMotion() || !AnimateWindow(hPage, 180, AW_SLIDE | AW_HOR_NEGATIVE))
        ShowWindow(hPage, SW_SHOW);
    SendMessageW(hPage, SPN_SETACTIVE, 0, 0);
    InvalidateRect(Frame.hwnd, NULL, FALSE);
}

VOID
SetupFrameGoTo(
    _In_ HWND hPage,
    _In_ INT Page)
{
    UNREFERENCED_PARAMETER(hPage);
    FrameShowPage(Page);
}

static BOOL
FrameSkipPage(
    _In_ INT Page)
{
    if (Frame.SetupData->bUnattend && (Frame.PageInfo[Page].Flags & SETUP_PAGE_SKIP_UNATTEND))
        return TRUE;
    return (Frame.PageInfo[Page].Flags & SETUP_PAGE_OPTIONAL) &&
           SendMessageW(Frame.Pages[Page], SPN_QUERYSKIP, 0, 0);
}

static INT
FrameNextPage(
    _In_ INT Page)
{
    while (Page < Frame.PageCount && FrameSkipPage(Page))
        Page++;
    return Page;
}

static VOID
FrameNext(VOID)
{
    HWND hNext = GetDlgItem(Frame.hwnd, IDC_FRAME_NEXT);
    INT Current = Frame.Current;
    INT Page;

    if (!IsWindowVisible(hNext) || !IsWindowEnabled(hNext))
        return;

    if (SendMessageW(Frame.Pages[Current], SPN_WIZNEXT, 0, 0) == -1 || Frame.Current != Current)
        return;

    Page = FrameNextPage(Current + 1);
    if (Page < Frame.PageCount)
        FrameShowPage(Page);
}

static VOID
FrameBack(VOID)
{
    HWND hBack = GetDlgItem(Frame.hwnd, IDC_FRAME_BACK);
    INT Current = Frame.Current;
    INT Page;

    if (!IsWindowVisible(hBack) || !IsWindowEnabled(hBack))
        return;

    if (SendMessageW(Frame.Pages[Current], SPN_WIZBACK, 0, 0) == -1 || Frame.Current != Current)
        return;

    Page = Current - 1;
    while (Page > 0 && FrameSkipPage(Page))
        Page--;
    if (Page >= 0)
        FrameShowPage(Page);
}

static VOID
FrameCancel(VOID)
{
    if (!IsWindowEnabled(GetDlgItem(Frame.hwnd, IDCANCEL)))
        return;

    if (Frame.PageInfo[Frame.Current].Flags & SETUP_PAGE_CLOSE)
    {
        EndDialog(Frame.hwnd, IDCANCEL);
        return;
    }

    if (SendMessageW(Frame.Pages[Frame.Current], SPN_QUERYCANCEL, 0, 0))
        return;

    if (SetupMessage(Frame.hwnd, TD_WARNING_ICON, TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, IDNO,
                     MAKEINTRESOURCEW(IDS_ABORTSETUP2), MAKEINTRESOURCEW(IDS_ABORTSETUP)) == IDYES)
    {
        EndDialog(Frame.hwnd, IDCANCEL);
    }
}

static VOID
FramePaintPanelEdge(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ INT cx,
    _In_ INT cy,
    _In_ INT Radius,
    _In_ COLORREF Color)
{
    BITMAPINFO bmi;
    PULONG Bits = NULL;
    HBITMAP hbm;
    HDC hdcMem;
    LONG Limit = (LONG)Radius * 8 * (LONG)Radius * 8;
    INT i, j, sx, sy;

    if (cx <= 0 || cy <= 0)
        return;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    hbm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (PVOID*)&Bits, NULL, 0);
    if (!hbm)
        return;
    for (j = 0; j < cy; j++)
    {
        for (i = 0; i < cx; i++)
        {
            ULONG Alpha = 255;

            if (i < Radius && j < Radius)
            {
                ULONG Inside = 0;

                for (sy = 0; sy < 4; sy++)
                {
                    for (sx = 0; sx < 4; sx++)
                    {
                        LONG dx = Radius * 8 - (i * 8 + sx * 2 + 1);
                        LONG dy = Radius * 8 - (j * 8 + sy * 2 + 1);

                        if (dx * dx + dy * dy <= Limit)
                            Inside++;
                    }
                }
                Alpha = Inside * 255 / 16;
            }
            Bits[(SIZE_T)j * cx + i] = (Alpha << 24) |
                                       ((GetRValue(Color) * Alpha / 255) << 16) |
                                       ((GetGValue(Color) * Alpha / 255) << 8) |
                                       (GetBValue(Color) * Alpha / 255);
        }
    }
    hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
    {
        HGDIOBJ hOld = SelectObject(hdcMem, hbm);

        BitBlt(hdc, x, y, cx, cy, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
    }
    DeleteObject(hbm);
}

static VOID
FramePaintColumn(
    _In_ HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT rc;
    HDC hdc = BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, Frame.Background);
    {
        HGDIOBJ OldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
        HPEN Pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HGDIOBJ OldPen = SelectObject(hdc, Pen);
        RoundRect(hdc, Frame.Watermark.cx + 12, 12, rc.right - 12, rc.bottom - 70, 24, 24);
        SelectObject(hdc, OldPen); SelectObject(hdc, OldBrush); DeleteObject(Pen);
    }
    rc.right = Frame.Watermark.cx;
    FillRect(hdc, &rc, Frame.Sidebar);

    if (Frame.hWatermark)
    {
        HDC hdcMem = CreateCompatibleDC(hdc);

        if (hdcMem)
        {
            HGDIOBJ hOld = SelectObject(hdcMem, Frame.hWatermark);

            BitBlt(hdc, 10, 88, 144, 122, hdcMem, 10, 98, SRCCOPY);
            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }
    }

    GetClientRect(hwnd, &rc);
    {
        WCHAR Text[96];
        HGDIOBJ Old = SelectObject(hdc, Frame.BodyFont);
        RECT Step = {18, rc.bottom - 78, Frame.Watermark.cx - 12, rc.bottom - 18};
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(53, 85, 119));
        StringCchPrintfW(Text, ARRAYSIZE(Text), L"THYFOS\nStep %d of %d", Frame.Current + 1, Frame.PageCount);
        DrawTextW(hdc, Text, -1, &Step, DT_LEFT | DT_WORDBREAK);
        SelectObject(hdc, Old);
    }

    EndPaint(hwnd, &ps);
}

static VOID
FrameCenter(
    _In_ HWND hwnd)
{
    RECT rcWork, rcWindow;

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
    GetWindowRect(hwnd, &rcWindow);
    SetWindowPos(hwnd, HWND_TOP,
                 rcWork.left + ((rcWork.right - rcWork.left) - (rcWindow.right - rcWindow.left)) / 2,
                 rcWork.top + ((rcWork.bottom - rcWork.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0, 0, SWP_NOSIZE);
}

static INT_PTR CALLBACK
SetupFrameProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            PSETUPDATA pSetupData = (PSETUPDATA)lParam;
            MARGINS Margins = {0, 0, 0, 0};
            HICON hIcon;
            BITMAP bm;
            RECT rc;
            INT Page;

            Frame.SetupData = pSetupData;
            Frame.hwnd = hwnd;
            Frame.Current = -1;
            Frame.Background = CreateSolidBrush(RGB(239, 245, 252));
            Frame.Sidebar = CreateSolidBrush(RGB(221, 234, 248));
            {
                LOGFONTW Font = {0};
                Font.lfHeight = -13; Font.lfWeight = FW_NORMAL;
                Font.lfCharSet = DEFAULT_CHARSET; Font.lfQuality = CLEARTYPE_QUALITY;
                StringCchCopyW(Font.lfFaceName, ARRAYSIZE(Font.lfFaceName), L"Selawik");
                Frame.BodyFont = CreateFontIndirectW(&Font);
            }
            SetWindowTextW(hwnd, L"THYFOS Setup");
            EnumChildWindows(hwnd, FrameSetFont, (LPARAM)Frame.BodyFont);
            SendDlgItemMessageW(hwnd, IDC_FRAME_NEXT, BM_SETSTYLE, BS_OWNERDRAW, FALSE);
            SendDlgItemMessageW(hwnd, IDC_FRAME_BACK, BM_SETSTYLE, BS_OWNERDRAW, FALSE);
            SendDlgItemMessageW(hwnd, IDCANCEL, BM_SETSTYLE, BS_OWNERDRAW, FALSE);

            hIcon = LoadIconW(pSetupData->hInstance, MAKEINTRESOURCEW(IDI_MAIN));
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            if (!pSetupData->hTitleFont)
                pSetupData->hTitleFont = CreateFrameFont(16);
            if (!pSetupData->hBoldFont)
                pSetupData->hBoldFont = CreateFrameFont(0);
            SetDlgItemFont(hwnd, IDC_FRAME_TITLE, pSetupData->hTitleFont, FALSE);

            Frame.hWatermark = LoadImageW(pSetupData->hInstance, MAKEINTRESOURCEW(IDB_WATERMARK),
                                          IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            if (Frame.hWatermark && GetObjectW(Frame.hWatermark, sizeof(bm), &bm))
            {
                Frame.Watermark.cx = bm.bmWidth;
                Frame.Watermark.cy = bm.bmHeight;
            }
            {
                HDC hdc = GetDC(hwnd);
                Frame.Corner = MulDiv(8, hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 96, 96);
                if (hdc)
                    ReleaseDC(hwnd, hdc);
            }
            Margins.cxLeftWidth = 0;
            Margins.cyTopHeight = 0;
            DwmExtendFrameIntoClientArea(hwnd, &Margins);
            {
                DWORD Corner = 2;
                DWORD Backdrop = DWMSBT_TRANSIENTWINDOW;
                DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &Corner, sizeof(Corner));
                DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &Backdrop, sizeof(Backdrop));
                FrameApplyGlass(hwnd);
            }

            GetWindowRect(GetDlgItem(hwnd, IDC_FRAME_PAGE), &rc);
            MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rc, 2);
            for (Page = 0; Page < Frame.PageCount; Page++)
            {
                Frame.Pages[Page] = CreateDialogParamW(pSetupData->hInstance,
                                                       MAKEINTRESOURCEW(Frame.PageInfo[Page].Template),
                                                       hwnd,
                                                       Frame.PageInfo[Page].Proc,
                                                       (LPARAM)pSetupData);
                if (!Frame.Pages[Page])
                {
                    EndDialog(hwnd, IDABORT);
                    return FALSE;
                }
                SetWindowPos(Frame.Pages[Page], HWND_TOP, rc.left, rc.top,
                             rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE);
                SetWindowSubclass(Frame.Pages[Page], FramePageProc, 1, 0);
                EnumChildWindows(Frame.Pages[Page], FrameSetFont, (LPARAM)Frame.BodyFont);
            }

            FrameCenter(hwnd);
            Page = (Frame.Restart >= 0) ? Frame.Restart : FrameNextPage(0);
            Frame.Restart = -1;
            FrameShowPage(Page);
            return FALSE;
        }

        case WM_PAINT:
            FramePaintColumn(hwnd);
            return TRUE;

        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            SetTextColor((HDC)wParam, RGB(29, 49, 73));
            SetBkColor((HDC)wParam, RGB(239, 245, 252));
            return (INT_PTR)Frame.Background;

        case WM_DRAWITEM:
            if (wParam == IDC_FRAME_NEXT || wParam == IDC_FRAME_BACK || wParam == IDCANCEL)
            { FrameDrawButton((const DRAWITEMSTRUCT *)lParam); return TRUE; }
            break;

        case WM_SETTINGCHANGE:
            FrameApplyGlass(hwnd);
            InvalidateRect(hwnd,NULL,TRUE);
            break;

        case WM_ACTIVATE:
            if (Frame.Current >= 0)
                SendMessageW(Frame.Pages[Frame.Current], WM_ACTIVATE, wParam, lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDC_FRAME_NEXT:
                    FrameNext();
                    return TRUE;
                case IDC_FRAME_BACK:
                    FrameBack();
                    return TRUE;
                case IDCANCEL:
                    FrameCancel();
                    return TRUE;
            }
            break;

        case WM_DESTROY:
            DwmSettingsClearContentBackdrop(hwnd);
            DeleteObject(Frame.Background); DeleteObject(Frame.Sidebar); DeleteObject(Frame.BodyFont);
            if (Frame.SetupData->hTitleFont)
                DeleteObject(Frame.SetupData->hTitleFont);
            if (Frame.SetupData->hBoldFont)
                DeleteObject(Frame.SetupData->hBoldFont);
            Frame.SetupData->hTitleFont = NULL;
            Frame.SetupData->hBoldFont = NULL;
            if (Frame.hWatermark)
                DeleteObject(Frame.hWatermark);
            Frame.hWatermark = NULL;
            break;
    }

    return FALSE;
}

VOID
SetupFrameReload(
    _In_ HWND hPage)
{
    UNREFERENCED_PARAMETER(hPage);
    Frame.Restart = Frame.Current;
    EndDialog(Frame.hwnd, IDRETRY);
}

VOID
SetSetupLanguage(
    _In_ PCWSTR LocaleId)
{
    LANGID LangId = LANGIDFROMLCID(wcstoul(LocaleId, NULL, 16));

    if (LangId && SetThreadUILanguage(LangId) != LangId)
        DPRINT1("SetThreadUILanguage(%04x) failed, error %lu\n", LangId, GetLastError());
}

INT_PTR
RunSetupFrame(
    _In_ PSETUPDATA pSetupData,
    _In_reads_(PageCount) const SETUP_PAGE_INFO* PageInfo,
    _In_ INT PageCount)
{
    INT_PTR Result;

    Frame.PageInfo = PageInfo;
    Frame.PageCount = min(PageCount, SETUP_MAX_PAGES);
    Frame.Restart = -1;
    do
    {
        Result = DialogBoxParamW(pSetupData->hInstance, MAKEINTRESOURCEW(IDD_SETUPFRAME),
                                 NULL, SetupFrameProc, (LPARAM)pSetupData);
    } while (Result == IDRETRY);
    return Result;
}
