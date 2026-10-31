/*
 * PROJECT:     THYFOS Desktop
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Glass desktop with an unpinned launcher and running-window dock
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <strsafe.h>
#include <wchar.h>
#include <shellframe.h>
#include "widgets.h"
#include "preferences.h"
#include <reactos/dwmframe.h>

static DesktopMeters Meters;
static BOOL AudioWidget=TRUE, ResourceWidget=TRUE;
static RECT ResourceCard, AudioCard;
static Gdiplus::Bitmap *Backdrop;
static HDC FrameDC;
static HBITMAP FrameBitmap;
static HGDIOBJ FrameOriginal;
static HWND Desktop, Search;
static HWND Tasks[12];
static int TaskCount, Width, Height;
static float Scale;
static BOOL Launcher;
static float LauncherMotion, LauncherFrom;
static DWORD LauncherStarted;
static RECT Dock, LauncherButton, Panel, Input, OpenButton, TaskRects[12], OldWorkArea;
static RECT AppRows[4];
static int ActiveApp=-1;
static HFONT BodyFont, ClockFont, HeadingFont;
static HBRUSH EditBrush;
static Gdiplus::Bitmap *Wallpaper;
static WCHAR WallpaperPath[MAX_PATH];
static UINT ShellMessage;
static HANDLE SetupProcess;
static DWORD SetupPid;
static DWORD WindowOpacity=85;
static const WCHAR OpacityOwner[]=L"THYFOS.WindowOpacity";
static const WCHAR CornerOwner[]=L"THYFOS.RoundedWindow";
static BOOL RoundedWindows=TRUE;

static void ApplyWindowCorners(HWND W)
{
    BOOL Owned=GetPropW(W,CornerOwner)!=NULL;
    ULONG_PTR Radius=(ULONG_PTR)GetPropW(W,DWM_PROP_CORNER_RADIUS);
    if (!RoundedWindows || IsZoomed(W) || IsIconic(W))
    {
        if (Owned)
        {
            if (Radius==14) RemovePropW(W,DWM_PROP_CORNER_RADIUS);
            RemovePropW(W,CornerOwner);
        }
        return;
    }
    if ((GetWindowLongPtrW(W,GWL_STYLE)&WS_CAPTION)!=WS_CAPTION ||
        (ULONG_PTR)GetPropW(W,DWM_PROP_WINDOW_CORNER_PREFERENCE)==DWM_CORNER_DONOTROUND) return;
    if (Radius) return;
    HRGN Shape=CreateRectRgn(0,0,0,0);
    if (!Shape) return;
    int Region=GetWindowRgn(W,Shape); DeleteObject(Shape);
    if (Region!=ERROR) return;
    if (SetPropW(W,CornerOwner,(HANDLE)1) && !SetPropW(W,DWM_PROP_CORNER_RADIUS,(HANDLE)14))
        RemovePropW(W,CornerOwner);
}

static void InitializeMotion()
{
    DWORD Initialized=0,Bytes=sizeof(Initialized);
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,L"MotionInitialized",RRF_RT_REG_DWORD,NULL,&Initialized,&Bytes);
    if (Initialized) return;
    ANIMATIONINFO Animation={sizeof(Animation)};
    if (SystemParametersInfoW(SPI_GETANIMATION,sizeof(Animation),&Animation,0) && Animation.iMinAnimate)
    {
        const UINT Actions[]={SPI_SETCLIENTAREAANIMATION,SPI_SETMENUANIMATION,SPI_SETMENUFADE,
            SPI_SETCOMBOBOXANIMATION,SPI_SETTOOLTIPANIMATION,SPI_SETTOOLTIPFADE,SPI_SETLISTBOXSMOOTHSCROLLING};
        for (UINT Action:Actions) SystemParametersInfoW(Action,0,(PVOID)TRUE,SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    }
    HKEY Key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,0,NULL,0,KEY_SET_VALUE,NULL,&Key,NULL)==ERROR_SUCCESS)
    { Initialized=1; RegSetValueExW(Key,L"MotionInitialized",0,REG_DWORD,(BYTE*)&Initialized,sizeof(Initialized)); RegCloseKey(Key); }
}

static void ApplyWindowOpacity(HWND W)
{
    LONG_PTR Style=GetWindowLongPtrW(W,GWL_EXSTYLE);
    BOOL Owned=GetPropW(W,OpacityOwner)!=NULL;
    if (WindowOpacity==100 || GetPropW(W,DWM_PROP_CONTENT_BACKDROP))
    {
        if (Owned)
        {
            SetWindowLongPtrW(W,GWL_EXSTYLE,Style&~WS_EX_LAYERED);
            RemovePropW(W,OpacityOwner);
            RedrawWindow(W,NULL,NULL,RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
        }
        return;
    }
    if (!(GetWindowLongPtrW(W,GWL_STYLE)&WS_CAPTION) ||
        (GetClassLongPtrW(W,GCL_STYLE)&(CS_OWNDC|CS_CLASSDC)) ||
        ((Style&WS_EX_LAYERED) && !Owned)) return;
    if (!Owned)
    {
        if (!SetPropW(W,OpacityOwner,(HANDLE)1)) return;
        SetLastError(ERROR_SUCCESS);
        if (!SetWindowLongPtrW(W,GWL_EXSTYLE,Style|WS_EX_LAYERED) && GetLastError()!=ERROR_SUCCESS)
        { RemovePropW(W,OpacityOwner); return; }
    }
    BYTE Alpha=(BYTE)((WindowOpacity*255+50)/100), Current;
    COLORREF Key; DWORD Flags;
    if (GetLayeredWindowAttributes(W,&Key,&Current,&Flags) && Flags==LWA_ALPHA && Current==Alpha) return;
    if (!SetLayeredWindowAttributes(W,0,Alpha,LWA_ALPHA))
    {
        SetWindowLongPtrW(W,GWL_EXSTYLE,Style&~WS_EX_LAYERED);
        RemovePropW(W,OpacityOwner);
    }
}

static int S(int V) { return (int)(V * Scale + 0.5f); }

static void Round(Gdiplus::Graphics &G, const RECT &R, float Radius, BYTE Alpha, COLORREF Color, BOOL Border = FALSE)
{
    Gdiplus::GraphicsPath P(Gdiplus::FillModeAlternate);
    float D = Radius * 2;
    P.AddArc((float)R.left, (float)R.top, D, D, 180, 90);
    P.AddArc(R.right-D, (float)R.top, D, D, 270, 90);
    P.AddArc(R.right-D, R.bottom-D, D, D, 0, 90);
    P.AddArc((float)R.left, R.bottom-D, D, D, 90, 90);
    P.CloseFigure();
    Gdiplus::Color C(Alpha, GetRValue(Color), GetGValue(Color), GetBValue(Color));
    if (Border) { Gdiplus::Pen Pen(C, 1.0f); G.DrawPath(&Pen, &P); }
    else { Gdiplus::SolidBrush Brush(C); G.FillPath(&Brush, &P); }
}

static void Label(HDC Dc, HFONT Font, PCWSTR Text, RECT R, UINT Align = DT_LEFT, COLORREF Ink = RGB(242,247,254))
{
    HGDIOBJ Old = SelectObject(Dc, Font);
    SetBkMode(Dc, TRANSPARENT);
    SetTextColor(Dc, Ink);
    DrawTextW(Dc, Text, -1, &R, Align | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(Dc, Old);
}

static void LoadWallpaper()
{
    delete Backdrop; Backdrop=NULL;
    delete Wallpaper;
    Wallpaper = NULL;
    WallpaperPath[0] = 0;
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, WallpaperPath, 0);
    if (!WallpaperPath[0])
    {
        GetWindowsDirectoryW(WallpaperPath, MAX_PATH);
        StringCchCatW(WallpaperPath, MAX_PATH, L"\\Web\\Wallpaper\\thyfos-wallpaper.bmp");
    }
    Wallpaper = Gdiplus::Bitmap::FromFile(WallpaperPath, FALSE);
    if (Wallpaper && Wallpaper->GetLastStatus() != Gdiplus::Ok)
    {
        delete Wallpaper;
        Wallpaper = NULL;
    }
}

static BOOL CALLBACK CollectTask(HWND W, LPARAM)
{
    if (W == Desktop || !IsWindowVisible(W) ||
        (GetWindowLongPtrW(W, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)) return TRUE;
    ApplyWindowOpacity(W);
    ApplyWindowCorners(W);
    if (GetWindow(W,GW_OWNER) || !GetWindowTextLengthW(W)) return TRUE;
    if (TaskCount < (int)_countof(Tasks)) Tasks[TaskCount++] = W;
    return TRUE;
}

static void Arrange()
{
    int D = S(76), DW = S(240 + TaskCount * 54);
    if (DW > Width - S(40)) DW = Width - S(40);
    SetRect(&Dock, (Width-DW)/2, Height-D-S(18), (Width+DW)/2, Height-S(18));
    SetRect(&LauncherButton, Dock.left+S(12), Dock.top+S(9), Dock.left+S(70), Dock.bottom-S(9));
    for (int I=0; I<TaskCount; ++I)
        SetRect(&TaskRects[I], Dock.left+S(86+I*54), Dock.top+S(10), Dock.left+S(134+I*54), Dock.bottom-S(10));
    SetRect(&Panel, S(24), max(S(80), Dock.top-S(510)), min(Width-S(24), S(464)), Dock.top-S(16));
    OffsetRect(&Panel,0,(int)(S(24)*(1.0f-LauncherMotion)));
    SetRect(&Input, Panel.left+S(20), Panel.top+S(24), Panel.right-S(20), Panel.top+S(72));
    SetRect(&OpenButton, Panel.right-S(120), Panel.bottom-S(66), Panel.right-S(20), Panel.bottom-S(24));
    int RowTop=Input.bottom+S(82);
    int RowStep=min(S(62),(OpenButton.top-S(12)-RowTop)/(int)_countof(AppRows));
    for (int I=0; I<(int)_countof(AppRows); ++I)
        SetRect(&AppRows[I],Panel.left+S(20),RowTop+I*RowStep,Panel.right-S(20),RowTop+(I+1)*RowStep-S(8));
    int CardWidth=min(S(420),Width-S(48));
    SetRect(&ResourceCard,Width-S(24)-CardWidth,Dock.top-S(174),Width-S(24),Dock.top-S(18));
    int AudioBottom=ResourceWidget?ResourceCard.top-S(14):Dock.top-S(18);
    SetRect(&AudioCard,Width-S(24)-CardWidth,AudioBottom-S(182),Width-S(24),AudioBottom);
    if (Search)
        SetWindowPos(Search, NULL, Input.left+S(16), Input.top+S(12), Input.right-Input.left-S(32),
                     S(26), SWP_NOZORDER | SWP_NOACTIVATE);
}

static void RefreshTasks()
{
    TaskCount = 0;
    EnumWindows(CollectTask, 0);
    Arrange();
    InvalidateRect(Desktop, NULL, FALSE);
}

static void Glass(Gdiplus::Graphics &G, const RECT &R, int Radius)
{
    Round(G, R, (float)S(Radius), (BYTE)((WindowOpacity*255+50)/100), RGB(42,64,88));
    Round(G, R, (float)S(Radius), 95, RGB(237,247,255), TRUE);
}

static BOOL FindChrome(PWSTR File, DWORD Count)
{
    const HKEY Roots[]={HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE};
    const PCWSTR Fallbacks[]={L"%ProgramFiles%\\Google\\Chrome\\Application\\chrome.exe",
                              L"%LOCALAPPDATA%\\Google\\Chrome\\Application\\chrome.exe"};
    WCHAR Value[MAX_PATH];
    for (HKEY Root:Roots)
    {
        DWORD Bytes=sizeof(Value);
        if (RegGetValueW(Root,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",NULL,
            RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND,NULL,Value,&Bytes)==ERROR_SUCCESS)
        {
            DWORD Length=ExpandEnvironmentStringsW(Value,File,Count);
            if (Length && Length<=Count)
            { PathUnquoteSpacesW(File); if (PathFileExistsW(File)) return TRUE; }
        }
    }
    for (PCWSTR Path:Fallbacks)
    {
        DWORD Length=ExpandEnvironmentStringsW(Path,File,Count);
        if (Length && Length<=Count && PathFileExistsW(File)) return TRUE;
    }
    File[0]=0; return FALSE;
}

static void Render(HDC Dc)
{
    Gdiplus::Graphics G(Dc);
    G.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    if (!Backdrop)
    {
        Backdrop=new Gdiplus::Bitmap(Width,Height,PixelFormat32bppARGB);
        Gdiplus::Graphics Background(Backdrop);
        Background.Clear(Gdiplus::Color(255,21,43,66));
        if (Wallpaper)
        {
            float F=max((float)Width/Wallpaper->GetWidth(),(float)Height/Wallpaper->GetHeight());
            Background.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            Background.DrawImage(Wallpaper,Gdiplus::RectF((Width-Wallpaper->GetWidth()*F)/2,
                (Height-Wallpaper->GetHeight()*F)/2,Wallpaper->GetWidth()*F,Wallpaper->GetHeight()*F));
        }
        Background.Flush(Gdiplus::FlushIntentionSync);
    }
    RECT Clip; GetClipBox(Dc,&Clip);
    G.DrawImage(Backdrop,Gdiplus::Rect(Clip.left,Clip.top,Clip.right-Clip.left,Clip.bottom-Clip.top),
        Clip.left,Clip.top,Clip.right-Clip.left,Clip.bottom-Clip.top,Gdiplus::UnitPixel);
    if (ResourceWidget)
    {
        Glass(G,ResourceCard,22);
        const double Values[]={Meters.Cpu,Meters.Ram,Meters.Gpu};
        const bool Valid[]={Meters.CpuValid,Meters.RamValid,Meters.GpuValid};
        const PCWSTR Names[]={L"CPU",L"RAM",L"GPU"};
        int Column=(ResourceCard.right-ResourceCard.left)/3;
        for (int I=0; I<3; ++I)
        {
            int X=ResourceCard.left+Column*I+(Column-S(64))/2,Y=ResourceCard.top+S(18);
            Gdiplus::Pen Track(Gdiplus::Color(70,219,235,255),(float)S(5));
            Gdiplus::Pen Fill(Gdiplus::Color(255,I==1?174:75,I==2?230:164,255),(float)S(5));
            G.DrawEllipse(&Track,X,Y,S(64),S(64));
            if (Valid[I]) G.DrawArc(&Fill,X,Y,S(64),S(64),-90.0f,(float)(Values[I]*3.6));
            G.Flush(Gdiplus::FlushIntentionSync);
            WCHAR Value[32];
            if (Valid[I]) StringCchPrintfW(Value,_countof(Value),L"%.0f%%",Values[I]);
            else StringCchCopyW(Value,_countof(Value),L"N/A");
            RECT R={X,Y,X+S(64),Y+S(64)}; Label(Dc,BodyFont,Value,R,DT_CENTER);
            R.top=Y+S(72); R.bottom=R.top+S(28); Label(Dc,BodyFont,Names[I],R,DT_CENTER);
        }
        WCHAR Memory[128];
        if (Meters.RamValid) StringCchPrintfW(Memory,_countof(Memory),L"RAM %.1f / %.1f GB  •  Open Task Manager",
            Meters.MemoryUsed/1073741824.0,Meters.MemoryTotal/1073741824.0);
        else StringCchCopyW(Memory,_countof(Memory),L"Memory unavailable  •  Open Task Manager");
        RECT R=ResourceCard; R.top=R.bottom-S(35); Label(Dc,BodyFont,Memory,R,DT_CENTER,RGB(203,218,235));
    }
    if (AudioWidget)
    {
        Glass(G,AudioCard,22); G.Flush(Gdiplus::FlushIntentionSync);
        RECT R=AudioCard; R.left+=S(20); R.top+=S(8); R.bottom=R.top+S(34);
        Label(Dc,BodyFont,Meters.AudioReady?L"Audio spectrum · System playback":L"Audio spectrum · Output unavailable",R);
        float Step=(float)(AudioCard.right-AudioCard.left-S(40))/ThyfosSpectrumBands;
        for (int I=0; I<ThyfosSpectrumBands; ++I)
        {
            int H=S(3)+(int)(Meters.Bands[I]*S(106));
            RECT Bar={AudioCard.left+S(20)+(int)(I*Step),AudioCard.bottom-S(22)-H,
                AudioCard.left+S(20)+(int)((I+1)*Step)-S(4),AudioCard.bottom-S(22)};
            Round(G,Bar,(float)min(S(3),H/2),220,RGB(176,218,255));
        }
    }
    WCHAR Time[64], Date[128];
    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, NULL, NULL, Time, _countof(Time));
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_LONGDATE, NULL, NULL, Date, _countof(Date));
    RECT Clock={Width-S(420), S(16), Width-S(28), S(72)};
    RECT Day={Width-S(420), S(72), Width-S(28), S(104)};
    RECT ClockCard={Width-S(430), S(10), Width-S(14), S(110)};
    Glass(G,ClockCard,18);
    G.Flush(Gdiplus::FlushIntentionSync);
    Label(Dc, ClockFont, Time, Clock, DT_RIGHT);
    Label(Dc, BodyFont, Date, Day, DT_RIGHT, RGB(203,218,235));
    Glass(G, Dock, 24);
    Round(G, LauncherButton, (float)S(16), Launcher?95:40, RGB(220,240,255));
    ShellFrame::FillIcon(G, L"apps_20_regular", Gdiplus::RectF((float)(LauncherButton.left+S(16)),
        (float)(LauncherButton.top+S(16)), (float)S(26), (float)S(26)), RGB(236,247,255));
    G.Flush(Gdiplus::FlushIntentionSync);
    if (!TaskCount)
    {
        RECT Empty={LauncherButton.right+S(14), Dock.top, Dock.right-S(16), Dock.bottom};
        Label(Dc, BodyFont, L"No pinned apps", Empty, DT_CENTER, RGB(196,213,232));
    }
    RECT Intersection;
    for (int I=0; I<TaskCount && IntersectRect(&Intersection,&Clip,&Dock); ++I)
    {
        DWORD_PTR Icon=0;
        SendMessageTimeoutW(Tasks[I], WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 50, &Icon);
        if (!Icon) Icon=GetClassLongPtrW(Tasks[I], GCLP_HICONSM);
        if (!Icon) Icon=(DWORD_PTR)LoadIconW(NULL, IDI_APPLICATION);
        DrawIconEx(Dc, TaskRects[I].left+S(8), TaskRects[I].top+S(8), (HICON)Icon, S(32), S(32), 0, NULL, DI_NORMAL);
        Gdiplus::SolidBrush Dot(Gdiplus::Color(220,225,241,255));
        G.FillEllipse(&Dot, (float)(TaskRects[I].left+S(22)), (float)(Dock.bottom-S(9)), (float)S(4), (float)S(4));
    }
    if (Launcher || LauncherMotion>0)
    {
        Glass(G, Panel, 26);
        Round(G, Input, (float)S(22), 245, RGB(50,71,94));
        Round(G, Input, (float)S(22), 65, RGB(245,250,255), TRUE);
        Round(G, OpenButton, (float)S(16), 70, RGB(220,240,255));
        G.Flush(Gdiplus::FlushIntentionSync);
        RECT Heading={Panel.left+S(28), Input.bottom+S(30), Panel.right-S(28), Input.bottom+S(72)};
        Label(Dc, HeadingFont, L"Launcher", Heading);
        WCHAR ChromePath[MAX_PATH];
        const PCWSTR Names[]={FindChrome(ChromePath,MAX_PATH)?L"Google Chrome":L"Install Google Chrome",L"File Explorer",L"Settings",L"THYFOS Store"};
        const PCWSTR Icons[]={L"globe_20_regular",L"folder_20_regular",L"settings_20_regular",L"apps_20_regular"};
        for (int I=0; I<(int)_countof(AppRows); ++I)
        {
            Round(G,AppRows[I],(float)S(16),I==ActiveApp?85:35,RGB(218,237,255));
            int IconSize=min(S(30),AppRows[I].bottom-AppRows[I].top-S(6));
            ShellFrame::FillIcon(G,Icons[I],Gdiplus::RectF((float)(AppRows[I].left+S(16)),
                (float)(AppRows[I].top+(AppRows[I].bottom-AppRows[I].top-IconSize)/2),(float)IconSize,(float)IconSize),RGB(228,241,255));
            G.Flush(Gdiplus::FlushIntentionSync);
            RECT Name=AppRows[I]; Name.left+=S(64);
            Label(Dc,BodyFont,Names[I],Name);
        }
        Label(Dc, BodyFont, L"Open", OpenButton, DT_CENTER);
    }
}

static void ConfigureWidgets()
{
    DWORD Value=1,Bytes=sizeof(Value);
    DWORD Opacity=85,OpacityBytes=sizeof(Opacity);
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_WINDOW_OPACITY,RRF_RT_REG_DWORD,NULL,&Opacity,&OpacityBytes);
    WindowOpacity=Opacity>=30 && Opacity<=100?Opacity:85;
    DWORD RoundValue=1,RoundBytes=sizeof(RoundValue);
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_ROUND_WINDOWS,RRF_RT_REG_DWORD,NULL,&RoundValue,&RoundBytes);
    RoundedWindows=RoundValue!=0;
    RefreshTasks();
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_AUDIO_WIDGET,RRF_RT_REG_DWORD,NULL,&Value,&Bytes);
    AudioWidget=Value!=0; Value=1; Bytes=sizeof(Value);
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_RESOURCE_WIDGET,RRF_RT_REG_DWORD,NULL,&Value,&Bytes);
    ResourceWidget=Value!=0; Value=1; Bytes=sizeof(Value);
    RegGetValueW(HKEY_CURRENT_USER,THYFOS_DESKTOP_KEY,THYFOS_REFRESH_SECONDS,RRF_RT_REG_DWORD,NULL,&Value,&Bytes);
    KillTimer(Desktop,2); KillTimer(Desktop,3);
    if (ResourceWidget) { Meters.SampleResources(); SetTimer(Desktop,2,max(1UL,min(5UL,Value))*1000,NULL); }
    if (AudioWidget) SetTimer(Desktop,3,50,NULL); else Meters.StopAudio();
    Arrange();
}

static void ToggleLauncher()
{
    Launcher=!Launcher;
    ActiveApp=-1;
    BOOL Motion=TRUE;
    ANIMATIONINFO Animation={sizeof(Animation)};
    SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&Motion,0);
    if (SystemParametersInfoW(SPI_GETANIMATION,sizeof(Animation),&Animation,0) && !Animation.iMinAnimate) Motion=FALSE;
    LauncherFrom=LauncherMotion; LauncherStarted=GetTickCount();
    KillTimer(Desktop,4);
    if (Motion) SetTimer(Desktop,4,16,NULL);
    else { LauncherMotion=Launcher?1.0f:0.0f; Arrange(); }
    ShowWindow(Search, Launcher?SW_SHOW:SW_HIDE);
    SetWindowPos(Desktop,Launcher?HWND_TOP:HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    InvalidateRect(Desktop, NULL, FALSE);
    if (Launcher) { SetForegroundWindow(Desktop); SetFocus(Search); }
}

static void OpenProgram(PCWSTR File, PCWSTR Args=NULL)
{
    HINSTANCE Result=ShellExecuteW(Desktop,L"open",File,Args,NULL,SW_SHOWNORMAL);
    if ((INT_PTR)Result<=32)
        MessageBoxW(Desktop,L"The app or path could not be opened. Check the name and try again.",L"THYFOS",MB_OK|MB_ICONINFORMATION);
    else { SetWindowTextW(Search,L""); if (Launcher) ToggleLauncher(); }
}

static void OpenSettings()
{
    WCHAR File[MAX_PATH];
    if (!GetWindowsDirectoryW(File,_countof(File)) ||
        FAILED(StringCchCatW(File,_countof(File),L"\\ImmersiveControlPanel\\SystemSettings.exe"))) return;
    OpenProgram(File);
}

static BOOL CALLBACK FocusSetup(HWND W, LPARAM)
{
    DWORD Pid; GetWindowThreadProcessId(W,&Pid);
    if (Pid==SetupPid && IsWindowVisible(W) && !GetWindow(W,GW_OWNER))
    { ShowWindow(W,SW_RESTORE); SetForegroundWindow(W); return FALSE; }
    return TRUE;
}

static void OpenSetup(BOOL Components)
{
    if (SetupProcess)
    {
        if (WaitForSingleObject(SetupProcess,0)==WAIT_TIMEOUT) { EnumWindows(FocusSetup,0); return; }
        CloseHandle(SetupProcess); SetupProcess=NULL;
    }
    WCHAR File[MAX_PATH], Command[MAX_PATH+32];
    GetWindowsDirectoryW(File,MAX_PATH);
    StringCchCatW(File,MAX_PATH,L"\\system32\\oobe\\oobe.exe");
    StringCchPrintfW(Command,_countof(Command),L"\"%s\"%s",File,Components?L" /components":L"");
    STARTUPINFOW Start={sizeof(Start)}; PROCESS_INFORMATION Process={};
    Start.dwFlags=STARTF_USESHOWWINDOW; Start.wShowWindow=SW_SHOWNORMAL;
    if (CreateProcessW(File,Command,NULL,NULL,FALSE,0,NULL,NULL,&Start,&Process))
    { SetupProcess=Process.hProcess; SetupPid=Process.dwProcessId; CloseHandle(Process.hThread); }
    else if (GetLastError()==ERROR_ELEVATION_REQUIRED)
    {
        SHELLEXECUTEINFOW Info={sizeof(Info)};
        Info.fMask=SEE_MASK_NOCLOSEPROCESS; Info.hwnd=Desktop; Info.lpVerb=L"runas";
        Info.lpFile=File; Info.lpParameters=Components?L"/components":NULL; Info.nShow=SW_SHOWNORMAL;
        if (ShellExecuteExW(&Info)) { SetupProcess=Info.hProcess; SetupPid=SetupProcess?GetProcessId(SetupProcess):0; }
    }
    if (!SetupProcess)
        MessageBoxW(Desktop,L"First-boot setup could not be started.",L"THYFOS Setup",MB_OK|MB_ICONERROR);
    else if (Launcher) ToggleLauncher();
}

static void RunFirstBootSetup()
{
    WCHAR Command[1024]; DWORD Bytes=sizeof(Command);
    if (RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",L"OOBE",
        RRF_RT_REG_SZ|RRF_RT_REG_EXPAND_SZ|RRF_NOEXPAND,NULL,Command,&Bytes)==ERROR_SUCCESS)
        OpenSetup(wcsstr(Command,L"/components")!=NULL);
}

static void OpenChrome()
{
    if (SetupProcess && WaitForSingleObject(SetupProcess,0)==WAIT_TIMEOUT) { OpenSetup(TRUE); return; }
    WCHAR File[MAX_PATH];
    if (FindChrome(File,MAX_PATH)) OpenProgram(File);
    else OpenSetup(TRUE);
}

static void OpenCommand()
{
    WCHAR Command[MAX_PATH*2], File[MAX_PATH*2], Args[MAX_PATH*2];
    GetWindowTextW(Search, Command, _countof(Command));
    if (!Command[0]) return;
    StrTrimW(Command,L" \t");
    if (!lstrcmpiW(Command,L"chrome") || !lstrcmpiW(Command,L"google chrome")) { OpenChrome(); return; }
    if (!lstrcmpiW(Command,L"settings") || !lstrcmpiW(Command,L"systemsettings") ||
        !lstrcmpiW(Command,L"systemsettings.exe")) { OpenSettings(); return; }
    if (!lstrcmpiW(Command,L"explorer") || !lstrcmpiW(Command,L"file explorer")) { OpenProgram(L"filebrowser.exe"); return; }
    if (!lstrcmpiW(Command,L"store") || !lstrcmpiW(Command,L"thyfos store")) { OpenProgram(L"thyfos-store.exe"); return; }
    StringCchCopyW(File, _countof(File), Command);
    Args[0]=0;
    if (!PathFileExistsW(Command))
    {
        StringCchCopyW(Args, _countof(Args), PathGetArgsW(Command));
        PathRemoveArgsW(File);
        PathUnquoteSpacesW(File);
    }
    OpenProgram(File,Args[0]?Args:NULL);
}

static WNDPROC OldEditProc;
static void OpenLauncherApp(int Index)
{
    if (Index==0) OpenChrome();
    else if (Index==1) OpenProgram(L"filebrowser.exe");
    else if (Index==2) OpenSettings();
    else if (Index==3) OpenProgram(L"thyfos-store.exe");
}

static LRESULT CALLBACK EditProc(HWND W, UINT M, WPARAM A, LPARAM B)
{
    if (M==WM_KEYDOWN && (A==VK_UP || A==VK_DOWN))
    {
        const int Count=(int)_countof(AppRows);
        ActiveApp=A==VK_DOWN?(ActiveApp+1)%Count:(ActiveApp<0?Count-1:(ActiveApp+Count-1)%Count);
        InvalidateRect(Desktop,&Panel,FALSE); return 0;
    }
    if (M==WM_KEYDOWN && A==VK_RETURN)
    { if (ActiveApp>=0) OpenLauncherApp(ActiveApp); else OpenCommand(); return 0; }
    if (M==WM_CHAR || M==WM_SETFOCUS)
    { ActiveApp=-1; InvalidateRect(Desktop,&Panel,FALSE); }
    if (M==WM_KEYDOWN && A==VK_ESCAPE) { if (Launcher) ToggleLauncher(); return 0; }
    LRESULT Result=CallWindowProcW(OldEditProc,W,M,A,B);
    if (M==WM_PAINT && !GetWindowTextLengthW(W))
    {
        RECT R; GetClientRect(W,&R);
        HDC Dc=GetDC(W); Label(Dc,BodyFont,L"App, file or folder...",R,DT_LEFT,RGB(182,202,223)); ReleaseDC(W,Dc);
    }
    return Result;
}

static void ContextMenu(POINT P)
{
    HMENU Menu=CreatePopupMenu();
    AppendMenuW(Menu,MF_STRING,1,L"Launcher");
    AppendMenuW(Menu,MF_STRING,2,L"Files");
    AppendMenuW(Menu,MF_STRING,5,L"Google Chrome");
    AppendMenuW(Menu,MF_STRING,3,L"Settings");
    AppendMenuW(Menu,MF_STRING,6,L"THYFOS Store");
    AppendMenuW(Menu,MF_SEPARATOR,0,NULL);
    AppendMenuW(Menu,MF_STRING,4,L"Sign out");
    int Id=TrackPopupMenu(Menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,P.x,P.y,0,Desktop,NULL);
    DestroyMenu(Menu);
    if (Id==1) ToggleLauncher();
    if (Id==2) ShellExecuteW(Desktop,L"open",L"filebrowser.exe",NULL,NULL,SW_SHOWNORMAL);
    if (Id==3) OpenSettings();
    if (Id==4) ExitWindowsEx(EWX_LOGOFF,0);
    if (Id==5) OpenChrome();
    if (Id==6) OpenProgram(L"thyfos-store.exe");
}

static LRESULT CALLBACK WindowProc(HWND W,UINT M,WPARAM A,LPARAM B)
{
    if (ShellMessage && M==ShellMessage) { RefreshTasks(); return 0; }
    switch (M)
    {
        case WM_CREATE:
            Desktop=W;
            Width=GetSystemMetrics(SM_CXSCREEN); Height=GetSystemMetrics(SM_CYSCREEN);
            Scale=max(0.75f,min(2.0f,Height/1080.0f));
            BodyFont=CreateFontW(-S(17),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Selawik");
            HeadingFont=CreateFontW(-S(25),0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Selawik");
            ClockFont=CreateFontW(-S(48),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Selawik Light");
            EditBrush=CreateSolidBrush(RGB(50,71,94));
            Search=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,0,0,W,(HMENU)100,
                GetModuleHandleW(NULL),NULL);
            SendMessageW(Search,WM_SETFONT,(WPARAM)BodyFont,FALSE);
            SendMessageW(Search,EM_SETCUEBANNER,TRUE,(LPARAM)L"App, file or folder...");
            OldEditProc=(WNDPROC)SetWindowLongPtrW(Search,GWLP_WNDPROC,(LONG_PTR)EditProc);
            InitializeMotion(); LoadWallpaper(); RefreshTasks(); ConfigureWidgets();
            SystemParametersInfoW(SPI_GETWORKAREA,0,&OldWorkArea,0);
            { RECT Work={0,0,Width,Dock.top-S(8)}; SystemParametersInfoW(SPI_SETWORKAREA,0,&Work,SPIF_SENDCHANGE); }
            ShellMessage=RegisterWindowMessageW(L"SHELLHOOK"); RegisterShellHookWindow(W);
            RegisterHotKey(W,1,MOD_CONTROL,VK_SPACE);
            RegisterHotKey(W,2,MOD_WIN,'I');
            SetTimer(W,1,60000,NULL);
            return 0;
        case WM_PAINT:
        {
            PAINTSTRUCT P; HDC Dc=BeginPaint(W,&P);
            if (!FrameDC)
            {
                FrameDC=CreateCompatibleDC(Dc); FrameBitmap=CreateCompatibleBitmap(Dc,Width,Height);
                FrameOriginal=SelectObject(FrameDC,FrameBitmap);
            }
            int Saved=SaveDC(FrameDC);
            IntersectClipRect(FrameDC,P.rcPaint.left,P.rcPaint.top,P.rcPaint.right,P.rcPaint.bottom);
            Render(FrameDC); BitBlt(Dc,P.rcPaint.left,P.rcPaint.top,P.rcPaint.right-P.rcPaint.left,
                P.rcPaint.bottom-P.rcPaint.top,FrameDC,P.rcPaint.left,P.rcPaint.top,SRCCOPY);
            RestoreDC(FrameDC,Saved); EndPaint(W,&P); return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_HOTKEY:
            if (A==1) ToggleLauncher();
            if (A==2) OpenSettings();
            return 0;
        case WM_TIMER:
            if (A==4)
            {
                float T=min(1.0f,(GetTickCount()-LauncherStarted)/180.0f);
                float Ease=T*T*(3.0f-2.0f*T);
                RECT Dirty=Panel;
                LauncherMotion=LauncherFrom+((Launcher?1.0f:0.0f)-LauncherFrom)*Ease;
                Arrange(); UnionRect(&Dirty,&Dirty,&Panel); InvalidateRect(W,&Dirty,FALSE);
                if (T>=1.0f) KillTimer(W,4);
            }
            else if (A==2) { Meters.SampleResources(); InvalidateRect(W,&ResourceCard,FALSE); }
            else if (A==3) { Meters.SampleAudio(); InvalidateRect(W,&AudioCard,FALSE); }
            else { RECT R={Width-S(430),S(10),Width,S(110)}; InvalidateRect(W,&R,FALSE); }
            return 0;
        case WM_SETTINGCHANGE:
            LoadWallpaper(); ConfigureWidgets(); InvalidateRect(W,NULL,FALSE); return 0;
        case WM_DISPLAYCHANGE:
            if (FrameDC)
            { SelectObject(FrameDC,FrameOriginal); DeleteObject(FrameBitmap); DeleteDC(FrameDC); FrameDC=NULL; }
            Width=GetSystemMetrics(SM_CXSCREEN); Height=GetSystemMetrics(SM_CYSCREEN);
            SetWindowPos(W,HWND_BOTTOM,0,0,Width,Height,SWP_NOACTIVATE);
            delete Backdrop; Backdrop=NULL; Arrange();
            { RECT Work={0,0,Width,Dock.top-S(8)}; SystemParametersInfoW(SPI_SETWORKAREA,0,&Work,0); }
            InvalidateRect(W,NULL,FALSE); return 0;
        case WM_LBUTTONUP:
        {
            POINT P={GET_X_LPARAM(B),GET_Y_LPARAM(B)};
            if (ResourceWidget && PtInRect(&ResourceCard,P) && !(Launcher && PtInRect(&Panel,P))) OpenProgram(L"taskmgr11.exe");
            else if (PtInRect(&LauncherButton,P)) ToggleLauncher();
            else if (Launcher && PtInRect(&OpenButton,P)) OpenCommand();
            else if (Launcher && PtInRect(&AppRows[0],P)) OpenChrome();
            else if (Launcher && PtInRect(&AppRows[1],P)) OpenProgram(L"filebrowser.exe");
            else if (Launcher && PtInRect(&AppRows[2],P)) OpenSettings();
            else if (Launcher && PtInRect(&AppRows[3],P)) OpenProgram(L"thyfos-store.exe");
            else
            {
                for (int I=0;I<TaskCount;++I) if (PtInRect(&TaskRects[I],P))
                { ShowWindow(Tasks[I],IsIconic(Tasks[I])?SW_RESTORE:SW_SHOW); SetForegroundWindow(Tasks[I]); }
                if (Launcher && !PtInRect(&Panel,P)) ToggleLauncher();
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            POINT P={GET_X_LPARAM(B),GET_Y_LPARAM(B)}; int Index=-1;
            if (Launcher) for (int I=0; I<(int)_countof(AppRows); ++I)
                if (PtInRect(&AppRows[I],P)) Index=I;
            if (Index!=ActiveApp) { ActiveApp=Index; InvalidateRect(W,&Panel,FALSE); }
            if (Index>=0) SetCursor(LoadCursorW(NULL,IDC_HAND));
            return 0;
        }
        case WM_CONTEXTMENU: { POINT P; GetCursorPos(&P); ContextMenu(P); return 0; }
        case WM_CTLCOLOREDIT:
            SetTextColor((HDC)A,RGB(237,246,255)); SetBkColor((HDC)A,RGB(50,71,94)); return (LRESULT)EditBrush;
        case WM_ACTIVATE:
            if (LOWORD(A)==WA_INACTIVE && Launcher)
            {
                Launcher=FALSE; LauncherMotion=0; KillTimer(W,4); ShowWindow(Search,SW_HIDE);
                Arrange(); InvalidateRect(W,NULL,FALSE);
            }
            if (!Launcher) SetWindowPos(W,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
            return 0;
        case WM_CLOSE: return 0;
        case WM_DESTROY:
            if (FrameDC)
            { SelectObject(FrameDC,FrameOriginal); DeleteObject(FrameBitmap); DeleteDC(FrameDC); FrameDC=NULL; }
            UnregisterHotKey(W,1);
            UnregisterHotKey(W,2);
            DeregisterShellHookWindow(W); KillTimer(W,1); KillTimer(W,2); KillTimer(W,3); KillTimer(W,4); Meters.StopAudio();
            delete Backdrop; Backdrop=NULL;
            SystemParametersInfoW(SPI_SETWORKAREA,0,&OldWorkArea,SPIF_SENDCHANGE);
            delete Wallpaper; Wallpaper=NULL;
            if (SetupProcess) CloseHandle(SetupProcess);
            DeleteObject(BodyFont); DeleteObject(ClockFont); DeleteObject(HeadingFont); DeleteObject(EditBrush);
            PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(W,M,A,B);
}

int WINAPI wWinMain(HINSTANCE Instance,HINSTANCE,LPWSTR,int)
{
    Gdiplus::GdiplusStartupInput Input;
    ULONG_PTR Token;
    if (Gdiplus::GdiplusStartup(&Token,&Input,NULL)!=Gdiplus::Ok) return 1;
    HRESULT Com=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    WNDCLASSW Class={};
    Class.lpfnWndProc=WindowProc; Class.hInstance=Instance;
    Class.hCursor=LoadCursorW(NULL,IDC_ARROW); Class.lpszClassName=L"THYFOS.Desktop";
    RegisterClassW(&Class);
    HWND W=CreateWindowExW(WS_EX_TOOLWINDOW,Class.lpszClassName,L"THYFOS",WS_POPUP,0,0,
        GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),NULL,NULL,Instance,NULL);
    if (!W) { Gdiplus::GdiplusShutdown(Token); return 1; }
    typedef BOOL (WINAPI *REGISTER_SHELL)(HWND);
    REGISTER_SHELL RegisterShell=(REGISTER_SHELL)GetProcAddress(GetModuleHandleW(L"user32.dll"),"SetShellWindow");
    if (RegisterShell && !GetShellWindow()) RegisterShell(W);
    ShowWindow(W,SW_SHOW); SetWindowPos(W,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    UpdateWindow(W);
    HANDLE Ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,L"msgina: ShellReadyEvent");
    if (!Ready) Ready=OpenEventW(EVENT_MODIFY_STATE,FALSE,L"Global\\msgina: ShellReadyEvent");
    if (Ready) { SetEvent(Ready); CloseHandle(Ready); }
    RunFirstBootSetup();
    MSG Message;
    while (GetMessageW(&Message,NULL,0,0)>0) { TranslateMessage(&Message); DispatchMessageW(&Message); }
    if (SUCCEEDED(Com)) CoUninitialize();
    Gdiplus::GdiplusShutdown(Token);
    return 0;
}
