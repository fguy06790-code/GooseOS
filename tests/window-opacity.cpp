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
    WindowOpacity=70; ApplyWindowOpacity(W);
    BYTE Alpha=0; DWORD Flags=0; COLORREF Key=0;
    if (!GetLayeredWindowAttributes(W,&Key,&Alpha,&Flags) || Alpha!=179 || Flags!=LWA_ALPHA) return 3;
    ApplyWindowOpacity(W);
    WindowOpacity=100; ApplyWindowOpacity(W);
    if (GetWindowLongPtrW(W,GWL_EXSTYLE)&WS_EX_LAYERED) return 4;
    if (GetPropW(W,OpacityOwner)) return 5;
    SetWindowLongPtrW(W,GWL_EXSTYLE,GetWindowLongPtrW(W,GWL_EXSTYLE)|WS_EX_LAYERED);
    SetLayeredWindowAttributes(W,0,220,LWA_ALPHA);
    WindowOpacity=70; ApplyWindowOpacity(W);
    if (!GetLayeredWindowAttributes(W,&Key,&Alpha,&Flags) || Alpha!=220 || GetPropW(W,OpacityOwner)) return 6;
    DestroyWindow(W); UnregisterClassW(C.lpszClassName,C.hInstance);
    puts("70% alpha, repeat application, solid restoration and app-owned alpha preservation passed");
    return 0;
}
