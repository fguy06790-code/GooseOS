/*
 * PROJECT:     THYFOS Desktop
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Task Manager counters and WASAPI loopback spectrum
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#include <initguid.h>
#include "widgets.h"
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <d3dkmthk.h>
#include <math.h>
#include <string.h>

static double ClampPercent(double V) { return V<0?0:V>100?100:V; }
static ULONGLONG TimeValue(const FILETIME &T)
{
    ULARGE_INTEGER U; U.LowPart=T.dwLowDateTime; U.HighPart=T.dwHighDateTime; return U.QuadPart;
}

struct GpuNodeSample { ULONGLONG Running=0; LONGLONG Clock=0; bool Valid=false; };
static GpuNodeSample GpuPrevious[8][32];
static LUID GpuIds[8]={};
static UINT PreviousAdapters=0;

static bool SampleGpu(double &Value)
{
    typedef NTSTATUS (APIENTRY *ENUM_FN)(D3DKMT_ENUMADAPTERS2 *);
    typedef NTSTATUS (APIENTRY *QUERY_FN)(const D3DKMT_QUERYSTATISTICS *);
    typedef NTSTATUS (APIENTRY *CLOSE_FN)(const D3DKMT_CLOSEADAPTER *);
    HMODULE Gdi=GetModuleHandleW(L"gdi32.dll");
    ENUM_FN Enumerate=(ENUM_FN)GetProcAddress(Gdi,"D3DKMTEnumAdapters2");
    QUERY_FN Query=(QUERY_FN)GetProcAddress(Gdi,"D3DKMTQueryStatistics");
    CLOSE_FN Close=(CLOSE_FN)GetProcAddress(Gdi,"D3DKMTCloseAdapter");
    LARGE_INTEGER Frequency;
    if (!Enumerate || !Query || !Close || !QueryPerformanceFrequency(&Frequency) || Frequency.QuadPart<=0)
        return false;
    D3DKMT_ADAPTERINFO Adapters[8]={};
    D3DKMT_ENUMADAPTERS2 E={}; E.NumAdapters=8; E.pAdapters=Adapters;
    if (Enumerate(&E)<0 || E.NumAdapters>8) return false;
    bool Changed=PreviousAdapters!=E.NumAdapters;
    for (UINT A=0; A<E.NumAdapters; ++A)
        Changed=Changed || GpuIds[A].LowPart!=Adapters[A].AdapterLuid.LowPart ||
                           GpuIds[A].HighPart!=Adapters[A].AdapterLuid.HighPart;
    if (Changed)
        for (UINT A=0; A<8; ++A) for (UINT N=0; N<32; ++N) GpuPrevious[A][N].Valid=false;
    PreviousAdapters=E.NumAdapters;
    bool Valid=false; Value=0;
    for (UINT A=0; A<E.NumAdapters; ++A)
    {
        GpuIds[A]=Adapters[A].AdapterLuid;
        D3DKMT_QUERYSTATISTICS Q={}; Q.Type=D3DKMT_QUERYSTATISTICS_ADAPTER; Q.AdapterLuid=GpuIds[A];
        if (Query(&Q)>=0)
        {
            ULONG Nodes=Q.QueryResult.AdapterInformation.NodeCount;
            if (Nodes>32) Nodes=32;
            for (ULONG N=0; N<Nodes; ++N)
            {
                GpuNodeSample &Old=GpuPrevious[A][N];
                ZeroMemory(&Q,sizeof(Q)); Q.Type=D3DKMT_QUERYSTATISTICS_NODE; Q.AdapterLuid=GpuIds[A];
                Q.QueryNode.NodeId=N;
                LARGE_INTEGER Now;
                if (Query(&Q)<0 || !QueryPerformanceCounter(&Now) ||
                    Q.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart<0)
                { Old.Valid=false; continue; }
                ULONGLONG Running=(ULONGLONG)Q.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
                if (Old.Valid && Running>=Old.Running && Now.QuadPart>Old.Clock)
                {
                    double Seconds=(double)(Now.QuadPart-Old.Clock)/Frequency.QuadPart;
                    double Usage=ClampPercent((Running-Old.Running)/(Seconds*10000000.0)*100.0);
                    if (Usage>Value) Value=Usage;
                    Valid=true;
                }
                Old.Running=Running; Old.Clock=Now.QuadPart; Old.Valid=true;
            }
        }
        else for (UINT N=0; N<32; ++N) GpuPrevious[A][N].Valid=false;
        D3DKMT_CLOSEADAPTER C={}; C.hAdapter=Adapters[A].hAdapter; Close(&C);
    }
    return Valid;
}

void DesktopMeters::SampleResources()
{
    FILETIME Idle, Kernel, User;
    CpuValid=false;
    if (GetSystemTimes(&Idle,&Kernel,&User))
    {
        ULONGLONG I=TimeValue(Idle), T=TimeValue(Kernel)+TimeValue(User);
        if (CpuPrimed && T>PreviousTotal && I>=PreviousIdle)
        { Cpu=ClampPercent(100.0*(1.0-(double)(I-PreviousIdle)/(T-PreviousTotal))); CpuValid=true; }
        PreviousIdle=I; PreviousTotal=T; CpuPrimed=true;
    }
    else CpuPrimed=false;
    MEMORYSTATUSEX M={sizeof(M)};
    RamValid=GlobalMemoryStatusEx(&M) && M.ullTotalPhys>0 && M.ullAvailPhys<=M.ullTotalPhys;
    if (RamValid)
    { MemoryTotal=M.ullTotalPhys; MemoryUsed=M.ullTotalPhys-M.ullAvailPhys; Ram=100.0*MemoryUsed/MemoryTotal; }
    GpuValid=SampleGpu(Gpu);
}

static void Transform(const float *Input,double *Real,double *Imaginary)
{
    const UINT N=ThyfosSpectrumSize;
    for (UINT I=0; I<N; ++I)
    { Real[I]=Input[I]*(0.5-0.5*cos(6.283185307179586*I/(N-1))); Imaginary[I]=0; }
    for (UINT I=1,J=0; I<N; ++I)
    {
        UINT Bit=N>>1;
        for (;J&Bit;Bit>>=1) J^=Bit;
        J^=Bit;
        if (I<J)
        { double T=Real[I]; Real[I]=Real[J]; Real[J]=T; }
    }
    for (UINT Length=2; Length<=N; Length<<=1)
    {
        double Angle=-6.283185307179586/Length, WR=cos(Angle), WI=sin(Angle);
        for (UINT I=0; I<N; I+=Length)
        {
            double R=1, Im=0;
            for (UINT J=0; J<Length/2; ++J)
            {
                UINT A=I+J,B=A+Length/2;
                double BR=Real[B]*R-Imaginary[B]*Im, BI=Real[B]*Im+Imaginary[B]*R;
                Real[B]=Real[A]-BR; Imaginary[B]=Imaginary[A]-BI;
                Real[A]+=BR; Imaginary[A]+=BI;
                double Next=R*WR-Im*WI; Im=R*WI+Im*WR; R=Next;
            }
        }
    }
}

void ThyfosSpectrum(const float *Left,const float *Right,UINT Rate,float *Bands)
{
    double LR[ThyfosSpectrumSize],LI[ThyfosSpectrumSize],RR[ThyfosSpectrumSize],RI[ThyfosSpectrumSize];
    if (!Rate) { ZeroMemory(Bands,ThyfosSpectrumBands*sizeof(float)); return; }
    Transform(Left,LR,LI); Transform(Right,RR,RI);
    double High=Rate/2.0; if (High>16000) High=16000;
    for (UINT Band=0; Band<ThyfosSpectrumBands; ++Band)
    {
        UINT First=(UINT)(60*pow(High/60.0,(double)Band/ThyfosSpectrumBands)*ThyfosSpectrumSize/Rate);
        UINT Last=(UINT)(60*pow(High/60.0,(double)(Band+1)/ThyfosSpectrumBands)*ThyfosSpectrumSize/Rate);
        if (First<1) First=1;
        if (Last<=First) Last=First+1;
        if (Last>ThyfosSpectrumSize/2) Last=ThyfosSpectrumSize/2;
        double Peak=0;
        for (UINT I=First; I<Last; ++I)
        {
            double Power=(LR[I]*LR[I]+LI[I]*LI[I]+RR[I]*RR[I]+RI[I]*RI[I])/2;
            if (Power>Peak) Peak=Power;
        }
        double DB=20*log10(sqrt(Peak)/(ThyfosSpectrumSize/2)+1e-7)+6;
        double Level=(DB+60)/60;
        Bands[Band]=(float)(Level<0?0:Level>1?1:Level);
    }
}

bool DesktopMeters::StartAudio()
{
    IMMDeviceEnumerator *Enumerator=NULL;
    IMMDevice *Device=NULL;
    HRESULT H=CoCreateInstance(CLSID_MMDeviceEnumerator,NULL,CLSCTX_INPROC_SERVER,
        IID_IMMDeviceEnumerator,(void**)&Enumerator);
    if (SUCCEEDED(H)) H=Enumerator->GetDefaultAudioEndpoint(eRender,eMultimedia,&Device);
    if (SUCCEEDED(H)) H=Device->Activate(IID_IAudioClient,CLSCTX_INPROC_SERVER,NULL,(void**)&Client);
    if (SUCCEEDED(H)) H=Client->GetMixFormat(&Format);
    if (SUCCEEDED(H))
    {
        WORD Tag=Format->wFormatTag;
        FloatSamples=Tag==WAVE_FORMAT_IEEE_FLOAT;
        bool Pcm=Tag==WAVE_FORMAT_PCM;
        if (Tag==WAVE_FORMAT_EXTENSIBLE && Format->cbSize>=sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX))
        {
            WAVEFORMATEXTENSIBLE *Extended=(WAVEFORMATEXTENSIBLE*)Format;
            FloatSamples=IsEqualGUID(Extended->SubFormat,KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
            Pcm=IsEqualGUID(Extended->SubFormat,KSDATAFORMAT_SUBTYPE_PCM);
        }
        if (!Format->nChannels || !Format->nSamplesPerSec ||
            (!FloatSamples && !Pcm) || (FloatSamples && Format->wBitsPerSample!=32) ||
            (Pcm && Format->wBitsPerSample!=8 && Format->wBitsPerSample!=16 &&
             Format->wBitsPerSample!=24 && Format->wBitsPerSample!=32)) H=AUDCLNT_E_UNSUPPORTED_FORMAT;
    }
    if (SUCCEEDED(H)) H=Client->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK,
                                           1000000,0,Format,NULL);
    if (SUCCEEDED(H)) H=Client->GetService(IID_IAudioCaptureClient,(void**)&Capture);
    if (SUCCEEDED(H)) H=Client->Start();
    if (Device) Device->Release();
    if (Enumerator) Enumerator->Release();
    if (FAILED(H)) { StopAudio(); return false; }
    AudioReady=true; return true;
}

void DesktopMeters::StopAudio()
{
    if (Client) Client->Stop();
    if (Capture) Capture->Release();
    if (Client) Client->Release();
    if (Format) CoTaskMemFree(Format);
    Capture=NULL; Client=NULL; Format=NULL; AudioReady=false; Samples=0; WritePosition=0;
    SecureZeroMemory(Left,sizeof(Left)); SecureZeroMemory(Right,sizeof(Right)); ZeroMemory(Bands,sizeof(Bands));
}

float DesktopMeters::ReadSample(const BYTE *Data) const
{
    if (FloatSamples) { float V; memcpy(&V,Data,sizeof(V)); return V!=V?0:V<-1?-1:V>1?1:V; }
    if (Format->wBitsPerSample==8) return ((int)Data[0]-128)/128.0f;
    if (Format->wBitsPerSample==16) { SHORT V; memcpy(&V,Data,sizeof(V)); return V/32768.0f; }
    if (Format->wBitsPerSample==24)
    { int V=Data[0]|(Data[1]<<8)|(Data[2]<<16); if (V&0x800000) V-=0x1000000; return V/8388608.0f; }
    INT32 V; memcpy(&V,Data,sizeof(V)); return (float)(V/2147483648.0);
}

void DesktopMeters::SampleAudio()
{
    ULONGLONG Now=GetTickCount64();
    if (!AudioReady)
    {
        if (Now-LastAudioAttempt<5000) return;
        LastAudioAttempt=Now;
        if (!StartAudio()) return;
    }
    UINT Frames=0;
    HRESULT H=Capture->GetNextPacketSize(&Frames);
    while (SUCCEEDED(H) && Frames)
    {
        BYTE *Data=NULL; DWORD Flags=0;
        H=Capture->GetBuffer(&Data,&Frames,&Flags,NULL,NULL);
        if (H==AUDCLNT_S_BUFFER_EMPTY) break;
        if (FAILED(H)) break;
        LastPacket=Now;
        if (Flags&AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) Samples=0;
        UINT Bytes=Format->wBitsPerSample/8;
        for (UINT F=0; F<Frames; ++F)
        {
            Left[WritePosition]=(Flags&AUDCLNT_BUFFERFLAGS_SILENT)?0:ReadSample(Data+F*Format->nBlockAlign);
            Right[WritePosition]=(Flags&AUDCLNT_BUFFERFLAGS_SILENT)?0:
                ReadSample(Data+F*Format->nBlockAlign+(Format->nChannels>1?Bytes:0));
            WritePosition=(WritePosition+1)%ThyfosSpectrumSize;
            if (Samples<ThyfosSpectrumSize) ++Samples;
        }
        H=Capture->ReleaseBuffer(Frames);
        if (SUCCEEDED(H)) H=Capture->GetNextPacketSize(&Frames);
    }
    if (FAILED(H)) { StopAudio(); return; }
    if (Now-LastPacket>200)
    {
        Samples=0;
        for (UINT I=0; I<ThyfosSpectrumBands; ++I) Bands[I]*=0.82f;
        return;
    }
    if (Samples==ThyfosSpectrumSize)
    {
        float L[ThyfosSpectrumSize],R[ThyfosSpectrumSize],Next[ThyfosSpectrumBands];
        for (UINT I=0; I<ThyfosSpectrumSize; ++I)
        { L[I]=Left[(WritePosition+I)%ThyfosSpectrumSize]; R[I]=Right[(WritePosition+I)%ThyfosSpectrumSize]; }
        ThyfosSpectrum(L,R,Format->nSamplesPerSec,Next);
        for (UINT I=0; I<ThyfosSpectrumBands; ++I)
            Bands[I]+=(Next[I]-Bands[I])*(Next[I]>Bands[I]?0.65f:0.18f);
    }
}
