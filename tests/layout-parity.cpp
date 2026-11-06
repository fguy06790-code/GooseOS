/*
 * PROJECT: THYFOS Goose layout differential tests
 * LICENSE: GPL-3.0-or-later
 * PURPOSE: Compare the Goose port against the original native layout
 * COPYRIGHT: Copyright 2026 LiberNT contributors
 */
#include <thyfos_goose.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
using std::min;
using std::max;
namespace original {
struct RECT { int32_t left,top,right,bottom; };
static int Width,Height,TaskCount;
static float Scale,LauncherMotion;
static bool ResourceWidget,Search=true;
static RECT Dock,LauncherButton,Panel,Input,OpenButton,ResourceCard,AudioCard,TaskRects[12],AppRows[5],SearchRect;
static int S(int V) { return (int)(V*Scale+0.5f); }
static void SetRect(RECT *r,int l,int t,int right,int b) { *r={l,t,right,b}; }
static void OffsetRect(RECT *r,int x,int y) { r->left+=x;r->right+=x;r->top+=y;r->bottom+=y; }
static void SetWindowPos(bool,void*,int x,int y,int w,int h,int) { SearchRect={x,y,x+w,y+h}; }
#define _countof(value) (sizeof(value)/sizeof((value)[0]))
#define SWP_NOZORDER 0
#define SWP_NOACTIVATE 0
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


}
static bool Same(const original::RECT &a,const desktop_Rect_g7 &b) {
    return a.left==b.left_g && a.top==b.top_g && a.right==b.right_g && a.bottom==b.bottom_g;
}
int main() {
    goose_init(0,nullptr);
    const int sizes[][2]={{800,600},{1024,768},{1280,800},{1920,1080},{2560,1440},{1366,768}};
    int cases=0;
    for (const auto &size:sizes) for(int count=0;count<=12;++count)
    for(float motion:{0.0f,0.5f,1.0f}) for(bool resources:{false,true}) {
        original::Width=size[0];original::Height=size[1];original::TaskCount=count;
        original::Scale=max(0.75f,min(2.0f,size[1]/1080.0f));
        original::LauncherMotion=motion;original::ResourceWidget=resources;original::Arrange();
        desktop_Layout_g7 got={};
        thyfos_arrange(size[0],size[1],original::Scale,count,motion,resources,&got);
        if (!Same(original::Dock,got.dock_g) || !Same(original::LauncherButton,got.launcher_g) ||
            !Same(original::Panel,got.panel_g) || !Same(original::Input,got.input_g) ||
            !Same(original::OpenButton,got.open_button_g) || !Same(original::ResourceCard,got.resources_g) ||
            !Same(original::AudioCard,got.audio_g) || !Same(original::SearchRect,got.search_g)) return 1;
        for(int i=0;i<count;++i) if(!Same(original::TaskRects[i],got.tasks_g.e[i])) return 2;
        for(int i=0;i<5;++i) if(!Same(original::AppRows[i],got.rows_g.e[i])) return 3;
        ++cases;
    }
    std::printf("Goose desktop layout matches original C++ across %d screen/task/motion/widget configurations\n",cases);
    return 0;
}
