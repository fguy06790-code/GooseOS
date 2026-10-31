#define wWinMain ThyfosApplicationMain
#include "desktop.cpp"
#undef wWinMain
#include <stdio.h>
int wmain()
{
    WNDCLASSW C={}; C.lpfnWndProc=DefWindowProcW; C.hInstance=GetModuleHandleW(NULL); C.lpszClassName=L"THYFOS.CornerTest";
    if (!RegisterClassW(&C)) return 1;
    HWND W=CreateWindowW(C.lpszClassName,L"Corners",WS_OVERLAPPEDWINDOW,0,0,200,100,NULL,NULL,C.hInstance,NULL);
    if (!W) return 2;
    ApplyWindowCorners(W);
    if ((ULONG_PTR)GetPropW(W,DWM_PROP_CORNER_RADIUS)!=14 || !GetPropW(W,CornerOwner)) return 3;
    RoundedWindows=FALSE; ApplyWindowCorners(W);
    if (GetPropW(W,DWM_PROP_CORNER_RADIUS) || GetPropW(W,CornerOwner)) return 4;
    RoundedWindows=TRUE; SetPropW(W,DWM_PROP_CORNER_RADIUS,(HANDLE)22); ApplyWindowCorners(W);
    if ((ULONG_PTR)GetPropW(W,DWM_PROP_CORNER_RADIUS)!=22 || GetPropW(W,CornerOwner)) return 5;
    RemovePropW(W,DWM_PROP_CORNER_RADIUS);
    SetPropW(W,DWM_PROP_WINDOW_CORNER_PREFERENCE,(HANDLE)DWM_CORNER_DONOTROUND); ApplyWindowCorners(W);
    if (GetPropW(W,DWM_PROP_CORNER_RADIUS)) return 6;
    RemovePropW(W,DWM_PROP_WINDOW_CORNER_PREFERENCE);
    SetWindowRgn(W,CreateRectRgn(0,0,180,80),FALSE); ApplyWindowCorners(W);
    if (GetPropW(W,DWM_PROP_CORNER_RADIUS)) return 7;
    DestroyWindow(W); UnregisterClassW(C.lpszClassName,C.hInstance);
    const int Sizes[][2]={{800,600},{1024,768},{1280,800},{1920,1080},{2560,1440}};
    for (const auto &Size:Sizes)
    {
        Width=Size[0]; Height=Size[1]; Scale=max(0.75f,min(2.0f,Height/1080.0f)); LauncherMotion=1;
        Arrange();
        for (const RECT &Row:AppRows)
            if (Row.left<Panel.left || Row.right>Panel.right || Row.top<Input.bottom || Row.bottom>OpenButton.top) return 8;
    }
    puts("Rounded-window policy, app overrides, custom regions and five launcher layouts passed");
    return 0;
}
