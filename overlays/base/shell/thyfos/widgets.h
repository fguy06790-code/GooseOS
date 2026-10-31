/*
 * PROJECT:     THYFOS Desktop
 * LICENSE:     GPL-3.0-or-later (https://spdx.org/licenses/GPL-3.0-or-later)
 * PURPOSE:     Live resource and system-output audio measurements
 * COPYRIGHT:   Copyright 2026 LiberNT contributors
 */
#pragma once
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

enum { ThyfosSpectrumSize=1024, ThyfosSpectrumBands=32 };
void ThyfosSpectrum(const float *Left,const float *Right,UINT Rate,float *Bands);

class DesktopMeters
{
public:
    bool CpuValid=false, RamValid=false, GpuValid=false, AudioReady=false;
    double Cpu=0, Ram=0, Gpu=0;
    ULONGLONG MemoryUsed=0, MemoryTotal=0;
    float Bands[ThyfosSpectrumBands]={};
    void SampleResources();
    void SampleAudio();
    void StopAudio();
private:
    ULONGLONG PreviousIdle=0, PreviousTotal=0, LastAudioAttempt=0, LastPacket=0;
    bool CpuPrimed=false;
    IAudioClient *Client=NULL;
    IAudioCaptureClient *Capture=NULL;
    WAVEFORMATEX *Format=NULL;
    bool FloatSamples=false;
    UINT WritePosition=0, Samples=0;
    float Left[ThyfosSpectrumSize]={}, Right[ThyfosSpectrumSize]={};
    bool StartAudio();
    float ReadSample(const BYTE *Data) const;
};
