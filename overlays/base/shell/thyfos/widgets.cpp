/*
 * PROJECT:     THYFOS Desktop
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Task Manager counters and WASAPI loopback spectrum
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#include <initguid.h>
#include "widgets.h"
#include <thyfos_goose.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <d3dkmthk.h>
#include <math.h>
#include <string.h>

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
                    double Usage=thyfos_gpu_percent(Running-Old.Running,
                        (ULONGLONG)(Now.QuadPart-Old.Clock),(ULONGLONG)Frequency.QuadPart);
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
        CpuValid=thyfos_cpu_percent(PreviousTotal,PreviousIdle,T,I,CpuPrimed,&Cpu)!=0;
        PreviousIdle=I; PreviousTotal=T; CpuPrimed=true;
    }
    else CpuPrimed=false;
    MEMORYSTATUSEX M={sizeof(M)};
    RamValid=GlobalMemoryStatusEx(&M) && thyfos_ram_percent(M.ullTotalPhys,M.ullAvailPhys,&Ram)!=0;
    if (RamValid)
    { MemoryTotal=M.ullTotalPhys; MemoryUsed=M.ullTotalPhys-M.ullAvailPhys; }
    GpuValid=SampleGpu(Gpu);
}

void ThyfosSpectrum(const float *Left,const float *Right,UINT Rate,float *Bands)
{
    static_assert(ThyfosSpectrumSize==1024 && ThyfosSpectrumBands==32, "Goose spectrum interface size mismatch");
    const sl_f32 LeftSlice={const_cast<float*>(Left),Rate?ThyfosSpectrumSize:0};
    const sl_f32 RightSlice={const_cast<float*>(Right),Rate?ThyfosSpectrumSize:0};
    const sl_f32 Output={Bands,ThyfosSpectrumBands};
    thyfos_spectrum(LeftSlice,RightSlice,Rate,Output);
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
    if (FloatSamples)
    { float V; memcpy(&V,Data,sizeof(V)); return thyfos_float_sample(V); }
    const sl_u8 Input={const_cast<BYTE*>(Data),Format->wBitsPerSample/8};
    return thyfos_pcm_sample(Input,Format->wBitsPerSample);
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
        const sl_f32 Current={Bands,ThyfosSpectrumBands}, Empty={NULL,0};
        thyfos_smooth_bands(Current,Empty,1);
        return;
    }
    if (Samples==ThyfosSpectrumSize)
    {
        float L[ThyfosSpectrumSize],R[ThyfosSpectrumSize],Next[ThyfosSpectrumBands];
        for (UINT I=0; I<ThyfosSpectrumSize; ++I)
        { L[I]=Left[(WritePosition+I)%ThyfosSpectrumSize]; R[I]=Right[(WritePosition+I)%ThyfosSpectrumSize]; }
        ThyfosSpectrum(L,R,Format->nSamplesPerSec,Next);
        const sl_f32 Current={Bands,ThyfosSpectrumBands}, Target={Next,ThyfosSpectrumBands};
        thyfos_smooth_bands(Current,Target,0);
    }
}
