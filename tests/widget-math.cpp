/*
 * PROJECT: THYFOS Goose widget tests
 * LICENSE: GPL-3.0-or-later
 * PURPOSE: Check migrated widget arithmetic against native behavior
 */
#include <thyfos_goose.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <initializer_list>

static float OriginalPcm(const uint8_t *data,unsigned bits) {
    if(bits==8) return ((int)data[0]-128)/128.0f;
    if(bits==16) { int16_t v;std::memcpy(&v,data,2);return v/32768.0f; }
    if(bits==24) { int v=data[0]|(data[1]<<8)|(data[2]<<16);if(v&0x800000)v-=0x1000000;return v/8388608.0f; }
    int32_t v;std::memcpy(&v,data,4);return (float)(v/2147483648.0);
}
static double Clamp(double v) { return v<0?0:v>100?100:v; }
int main() {
    goose_init(0,nullptr);
    unsigned samples=0;
    uint32_t state=1;
    for(unsigned bits:{8u,16u,24u,32u}) {
        const uint32_t sign=uint32_t(1)<<(bits-1);
        const uint32_t maximum=bits==32?0xffffffffu:(uint32_t(1)<<bits)-1;
        for(uint32_t word:{0u,sign-1,sign,maximum}) {
            uint8_t bytes[4]={(uint8_t)word,(uint8_t)(word>>8),(uint8_t)(word>>16),(uint8_t)(word>>24)};
            if(thyfos_pcm_sample(sl_u8{bytes,bits/8},bits)!=OriginalPcm(bytes,bits)) return 13;
            ++samples;
        }
        const unsigned count=bits==8?256:bits==16?65536:100000;
        for(unsigned i=0;i<count;++i) {
            state=state*1664525u+1013904223u;
            uint32_t word=bits<=16?i:state;
            uint8_t bytes[4]={(uint8_t)word,(uint8_t)(word>>8),(uint8_t)(word>>16),(uint8_t)(word>>24)};
            if(thyfos_pcm_sample(sl_u8{bytes,bits/8},bits)!=OriginalPcm(bytes,bits)) return 1;
            ++samples;
        }
    }
    for(float v:{-INFINITY,-2.0f,-1.0f,-0.0f,0.0f,0.5f,1.0f,2.0f,INFINITY,std::numeric_limits<float>::quiet_NaN()}) {
        float expected=v!=v?0:v<-1?-1:v>1?1:v;
        const float got=thyfos_float_sample(v);
        if(got!=expected || (got==0 && std::signbit(got)!=std::signbit(expected))) return 2;
    }
    double output=42;
    if(thyfos_cpu_percent(10,2,10,2,1,&output) || output!=42) return 3;
    if(thyfos_cpu_percent(10,2,20,1,1,&output) || output!=42) return 4;
    if(thyfos_cpu_percent(10,2,20,3,0,&output) || output!=42) return 5;
    if(thyfos_ram_percent(0,0,&output) || output!=42 || thyfos_ram_percent(10,11,&output)) return 6;
    if(!thyfos_cpu_percent(10,0,20,30,1,&output) || output!=0) return 14;
    if(!thyfos_cpu_percent(UINT64_MAX-10,0,UINT64_MAX,0,1,&output) || output!=100) return 15;
    if(!thyfos_ram_percent(UINT64_MAX,0,&output) || output!=100) return 16;
    for(uint64_t delta=1;delta<10000;delta+=7) {
        const uint64_t idle=delta/3;
        if(!thyfos_cpu_percent(100,20,100+delta,20+idle,1,&output) ||
           output!=Clamp(100.0*(1.0-(double)idle/delta))) return 7;
        if(!thyfos_ram_percent(delta,idle,&output) || output!=100.0*(delta-idle)/delta) return 8;
        double expected=Clamp(delta/((delta/3000000.0)*10000000.0)*100.0);
        if(thyfos_gpu_percent(delta,delta,3000000)!=expected) return 9;
    }
    if(thyfos_gpu_percent(1,0,100)!=0 || thyfos_gpu_percent(1,10,0)!=0) return 10;
    float current[34]={},next[32]={},expected[32]={};
    current[32]=current[33]=17;
    for(int i=0;i<32;++i) { current[i]=expected[i]=i/32.0f;next[i]=(31-i)/32.0f; }
    for(int step=0;step<100;++step) {
        bool stale=step%3==0;
        for(int i=0;i<32;++i) {
            if(stale) expected[i]*=0.82f;
            else expected[i]+=(next[i]-expected[i])*(next[i]>expected[i]?0.65f:0.18f);
        }
        thyfos_smooth_bands(sl_f32{current,34},sl_f32{next,stale?0:32},stale);
        for(int i=0;i<32;++i) if(current[i]!=expected[i]) return 11;
        if(current[32]!=17 || current[33]!=17) return 12;
    }
    std::printf("Goose widget math passed %u PCM samples, counter boundaries, float sanitization and spectrum smoothing\n",samples);
}
