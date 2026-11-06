/*
 * PROJECT:     THYFOS Goose integration tests
 * LICENSE:     GPL-3.0-or-later
 * PURPOSE:     Exercise generated Goose exports from a native host
 */
#include <thyfos_goose.h>
#include <cmath>
#include <cstdio>

int main()
{
    goose_init(0, nullptr);
    const uint32_t transient[] = {1231,2250,12002,12007,12029,12030,12031};
    for (uint32_t code : transient)
        if (!thyfos_catalog_retryable(code)) return 1;
    const uint32_t rejected[] = {0,5,11,13,23,12037,12038,12045,0xffffffffu};
    for (uint32_t code : rejected)
        if (thyfos_catalog_retryable(code)) return 2;

    float left[1024]={},right[1024]={},bands[34];
    for (float &value : bands) value=-1;
    const sl_f32 l={left,1024},r={right,1024},out={bands,34};
    thyfos_spectrum(l,r,48000,out);
    for (int i=0;i<32;++i) if (bands[i]!=0) return 3;
    if (bands[32]!=-1 || bands[33]!=-1) return 4;
    for (int i=0;i<1024;++i)
    {
        left[i]=static_cast<float>(0.25*std::sin(6.283185307179586*1000*i/48000));
        right[i]=left[i];
    }
    thyfos_spectrum(l,r,48000,out);
    float reference[32];
    for (int i=0;i<32;++i) reference[i]=bands[i];
    for (int i=0;i<1024;++i) right[i]=-left[i];
    thyfos_spectrum(l,r,48000,out);
    int peak=0;
    for (int i=0;i<32;++i)
    {
        if (!std::isfinite(bands[i]) || bands[i]<0 || bands[i]>1) return 5;
        if (std::fabs(bands[i]-reference[i])>1e-6) return 6;
        if (bands[i]>bands[peak]) peak=i;
    }
    if (peak!=16 || std::fabs(bands[peak]-0.78840053f)>1e-6f)
    {
        std::printf("Unexpected 1 kHz peak: band=%d level=%.9g\n",peak,bands[peak]);
        return 7;
    }
    const sl_f32 empty={nullptr,0};
    thyfos_spectrum(empty,empty,0,out);
    for (int i=0;i<32;++i) if (bands[i]!=0) return 8;
    if (bands[32]!=-1 || bands[33]!=-1) return 9;
    std::puts("Native host calls actual Goose policy and spectrum exports; initialization, stereo phase, bounds and zero-rate handling passed");
    return 0;
}
