#define wWinMain ThyfosApplicationMain
#include "desktop.cpp"
#undef wWinMain
#include <stdio.h>
int wmain()
{
    WNDCLASSW C={}; C.lpfnWndProc=DefWindowProcW; C.hInstance=GetModuleHandleW(NULL); C.lpszClassName=L"THYFOS.OpacityTest";
    if (!RegisterClassW(&C)) return 1;
    HWND W=CreateWindowW(C.lpszClassName,L"Opacity test",WS_OVERLAPPEDWINDOW,0,0,200,100,NULL,NULL,C.hInstance,NULL);
    if (!W) return 2;
    WindowOpacity=85; ApplyWindowOpacity(W);
    BYTE Alpha=0; DWORD Flags=0; COLORREF Key=0;
    if (!GetLayeredWindowAttributes(W,&Key,&Alpha,&Flags) || Alpha!=217 || Flags!=LWA_ALPHA) return 3;
    ApplyWindowOpacity(W);
    WindowOpacity=100; ApplyWindowOpacity(W);
    if (GetWindowLongPtrW(W,GWL_EXSTYLE)&WS_EX_LAYERED) return 4;
    if (GetPropW(W,OpacityOwner)) return 5;
    SetWindowLongPtrW(W,GWL_EXSTYLE,GetWindowLongPtrW(W,GWL_EXSTYLE)|WS_EX_LAYERED);
    SetLayeredWindowAttributes(W,0,220,LWA_ALPHA);
    WindowOpacity=85; ApplyWindowOpacity(W);
    if (!GetLayeredWindowAttributes(W,&Key,&Alpha,&Flags) || Alpha!=220 || GetPropW(W,OpacityOwner)) return 6;
    SetWindowLongPtrW(W,GWL_EXSTYLE,GetWindowLongPtrW(W,GWL_EXSTYLE)&~WS_EX_LAYERED);
    ApplyWindowOpacity(W);
    SetPropW(W,DWM_PROP_CONTENT_BACKDROP,(HANDLE)1);
    ApplyWindowOpacity(W);
    if ((GetWindowLongPtrW(W,GWL_EXSTYLE)&WS_EX_LAYERED) || GetPropW(W,OpacityOwner)) return 7;
    if (!GetPropW(W,DWM_PROP_CONTENT_BACKDROP)) return 8;
    RemovePropW(W,DWM_PROP_CONTENT_BACKDROP);
    DestroyWindow(W); UnregisterClassW(C.lpszClassName,C.hInstance);
    puts("85% opacity, solid restoration, app alpha preservation and single glass blending passed");
    return 0;
}
