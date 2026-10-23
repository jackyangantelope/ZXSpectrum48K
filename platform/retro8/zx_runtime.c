/* SPDX-License-Identifier: BSD-3-Clause */

#include "retro8.h"
#include <string.h>
#include <ctype.h>
#if R8_ZX_SPLIT_PROFILE
static r8_zx_split_stats split_stats;
void r8_zx_split_get(r8_zx_split_stats *out){*out=split_stats;}
void r8_zx_split_reset(void){memset(&split_stats,0,sizeof split_stats);}
static void split_scanline(unsigned line,unsigned top,uint32_t elapsed){
    split_stats.ula_us+=elapsed;split_stats.calls++;
    if(elapsed>split_stats.max_line_us)split_stats.max_line_us=elapsed;
    if(line>=top-32&&line<top+192+32){
        if(line>=top&&line<top+192){split_stats.active_us+=elapsed;split_stats.active_rows++;}
        else{split_stats.border_us+=elapsed;split_stats.border_rows++;}
    }else{split_stats.blank_us+=elapsed;split_stats.blank_rows++;}
}
#define CHIPS_ZX_SCANLINE_BEGIN() r8_zx_profile_now()
#define CHIPS_ZX_SCANLINE_END(line,top,stamp) split_scanline((line),(top),(uint32_t)(r8_zx_profile_now()-(stamp)))
#endif
#if R8_SRAM_HOT

#define CHIPS_Z80_TICK_ATTR __attribute__((section(".time_critical.r8_z80_tick")))
#define CHIPS_Z80_FETCH_ATTR __attribute__((section(".time_critical.r8_z80_fetch")))
#define CHIPS_Z80_CONST_ATTR __attribute__((section(".time_critical.r8_z80_tables")))
#define CHIPS_ZX_TICK_ATTR __attribute__((section(".time_critical.r8_zx_tick")))
#define R8_FRAME_ATTR __attribute__((section(".time_critical.r8_frame")))
#else
#define R8_FRAME_ATTR
#endif
#if R8_FUSED_TICK

#define CHIPS_ZX_TICK_ATTR inline __attribute__((always_inline))
#endif
#if R8_Z80_INLINE
#define CHIPS_Z80_TICK_ATTR inline __attribute__((always_inline))
#endif
#define CHIPS_IMPL
#include "chips/chips_common.h"
#include "chips/z80.h"
#include "chips/beeper.h"
#include "chips/ay38910.h"
#include "chips/mem.h"
#include "chips/kbd.h"
#include "chips/clk.h"
#include "systems/zx.h"
static zx_t machine;
#if R8_BEEPER_STEADY
void r8_beeper_get(r8_beeper_stats *out){
    out->fast_ticks=machine.beeper.steady.fast_ticks;
    out->slow_ticks=machine.beeper.steady.slow_ticks;
    out->entries=machine.beeper.steady.entries;
}
#endif
static uint8_t hotram[3][16384];
static uint8_t *storage;
static r8_audio_fn audio_sink;
static r8_service_fn service_sink;
static bool loaded, snake_keys;
static uint8_t held[256];
static uint32_t audio_phase;
static float previous_sample;
static const char *error_text="";
static unsigned word(const uint8_t *p) { return p[0]|((unsigned)p[1]<<8); }
void retro8_service(void) { if(service_sink)service_sink(); }
static void pcm(const float *samples,int n,void *user) {
    (void)user;
    if(audio_sink)for(int i=0;i<n;i++) {

        /* Preserve the original beeper waveform while matching 44100 Hz HDMI audio. */
        const uint32_t step=44100u*(unsigned)machine.beeper.period;
        const uint32_t limit=3500000u*BEEPER_FIXEDPOINT_SCALE;
        audio_phase+=step;
        float current=samples[i];
        if(audio_phase<limit){previous_sample=current;continue;}
        audio_phase-=limit;
        float v=(current+(previous_sample-current)*((float)audio_phase/step))*32767.0f;
        previous_sample=current;
        if(v>32767)v=32767;
        if(v<-32768)v=-32768;
        audio_sink((int16_t)v,(int16_t)v);
    }
}
bool r8_init(void *bulk,size_t capacity,r8_audio_fn audio,r8_service_fn service) {
    if(!bulk||capacity<327680)return false;
    storage=bulk;audio_sink=audio;service_sink=service;return true;
}

static bool expand(const uint8_t *src,size_t size,uint8_t *out,size_t length,bool v1) {
    size_t i=0,j=0;
    while(i<size&&j<length) {
        if(i+1<size&&src[i]==0xED&&src[i+1]==0xED) {
            if(size-i<4||src[i+2]==0||src[i+2]>length-j)return false;
            memset(out+j,src[i+3],src[i+2]);j+=src[i+2];i+=4;
        } else out[j++]=src[i++];
    }
    if(j!=length)return false;
    if(i==size)return true;
    return v1&&size-i==4&&src[i]==0&&src[i+1]==0xED&&src[i+2]==0xED&&src[i+3]==0;
}
bool r8_load(const uint8_t *game,size_t size,const uint8_t *bios,size_t bios_size,const char *name) {
    loaded=false;
    if(!storage||!bios||bios_size!=16384) {error_text="Missing 16384-byte zx48.rom";return false;}
    if(!game||size<30||size>200000){error_text="Invalid Z80 snapshot size";return false;}
    uint8_t *ram=storage+262144;
    unsigned pc=word(game+6),flags=game[12]==255?1:game[12];
    if(pc) {
        if(flags&32) {if(!expand(game+30,size-30,ram,49152,true))goto bad;}
        else {if(size!=30+49152)goto bad;memcpy(ram,game+30,49152);}
    } else {
        if(size<35)goto bad;
        unsigned ext=word(game+30),hw=game[34];
        if(ext!=23&&ext!=54&&ext!=55)goto bad;
        if((ext==23&&hw>1)||(ext!=23&&hw!=0&&hw!=1&&hw!=3)) {
            error_text="Only Spectrum 48K snapshots supported";return false;
        }
        if(size<32u+ext)goto bad;

        if(game[37]&0x80)goto bad;
        pc=word(game+32);size_t pos=32+ext;unsigned pages=0;
        while(pos<size) {
            if(size-pos<3)goto bad;
            unsigned len=word(game+pos),page=game[pos+2];pos+=3;
            unsigned bank=page==8?0:page==4?1:page==5?2:99;
            if(bank==99||(pages&(1u<<bank)))goto bad;
            pages|=1u<<bank;
            unsigned bytes=len==0xFFFF?16384:len;
            if(bytes>size-pos)goto bad;
            if(len==0xFFFF)memcpy(ram+bank*16384,game+pos,16384);
            else if(!expand(game+pos,bytes,ram+bank*16384,16384,false))goto bad;
            pos+=bytes;
        }
        if(pages!=7)goto bad;
    }
    if((game[29]&3)>2)goto bad;
    zx_desc_t desc={0};desc.type=ZX_TYPE_48K;desc.joystick_type=ZX_JOYSTICKTYPE_KEMPSTON;
    desc.audio.callback.func=pcm;desc.audio.num_samples=128;desc.audio.sample_rate=44100;
    desc.roms.zx48k.ptr=(void *)bios;desc.roms.zx48k.size=16384;
#ifndef R8_CHIPS_REFERENCE
    for(unsigned i=0;i<8;i++)desc.backing.ram[i]=i<3?hotram[i]:storage+(i-3)*16384;
    desc.backing.rom[0]=storage+81920;desc.backing.rom[1]=storage+98304;
    desc.backing.junk=storage+114688;desc.backing.fb=storage+131072;
#endif
    zx_init(&machine,&desc);

    kbd_register_key(&machine.kbd,0x10,0,0,0);
    for(unsigned i=0;i<3;i++)memcpy(machine.ram[i],ram+16384*i,16384);
    z80_t *c=&machine.cpu;
    c->a=game[0];c->f=game[1];c->c=game[2];c->b=game[3];c->l=game[4];c->h=game[5];
    c->sp=word(game+8);c->i=game[10];c->r=(game[11]&127)|((flags&1)<<7);
    c->e=game[13];c->d=game[14];c->bc2=word(game+15);c->de2=word(game+17);
    c->hl2=word(game+19);c->af2=((unsigned)game[21]<<8)|game[22];
    c->iy=word(game+23);c->ix=word(game+25);c->iff1=game[27]!=0;c->iff2=game[28]!=0;c->im=game[29]&3;
    machine.pins=z80_prefetch(c,(uint16_t)pc);machine.border_color=(flags>>1)&7;
    memset(held,0,sizeof held);snake_keys=name&&strstr(name,"snake");
    audio_phase=0;previous_sample=0;
#if R8_ZX_SPLIT_PROFILE
    r8_zx_split_reset();
#endif
    loaded=true;error_text="";return true;
bad:error_text="Invalid/truncated 48K Z80 snapshot";return false;
}
static unsigned hid(unsigned h) {
    if(h>=4&&h<=29)return 'a'+h-4;
    if(h>=30&&h<=38)return '1'+h-30;
    switch(h){case 39:return '0';case 40:return 13;case 42:return 12;case 44:return ' ';
        case 45:return '-';case 46:return '=';case 51:return ';';case 52:return '\'';
        case 54:return ',';case 55:return '.';case 56:return '/';default:return 0;}
}
void r8_controls(const r8_input *state) {
    if(!loaded)return;
    uint8_t keys[256]={0};unsigned pad=state->pads[0];
    for(unsigned i=0;i<6;i++) {
        unsigned h=state->keys[i],k=hid(h);if(k)keys[k]=1;
        if(h==0x52)pad|=R8_UP;
        if(h==0x51)pad|=R8_DOWN;
        if(h==0x50)pad|=R8_LEFT;
        if(h==0x4F)pad|=R8_RIGHT;
    }
    if(state->modifiers&0x22)keys[0x10]=1;
    if(state->modifiers&0x11)keys[0x0F]=1;
    if(pad&R8_UP)keys[snake_keys?'w':'q']=1;
    if(pad&R8_DOWN)keys[snake_keys?'s':'a']=1;
    if(pad&R8_LEFT)keys[snake_keys?'a':'o']=1;
    if(pad&R8_RIGHT)keys[snake_keys?'d':'p']=1;
    if(pad&R8_A)keys[' ']=1;
    if(pad&R8_B)keys['w']=1;
    if(pad&R8_X)keys['i']=1;
    if(pad&R8_Y)keys['s']=1;
    if(pad&R8_START)keys[13]=1;
    if(pad&R8_SELECT)keys['g']=1;
    for(unsigned i=1;i<256;i++) {
        if(keys[i])kbd_key_down(&machine.kbd,(int)i);
        else if(held[i])kbd_key_up(&machine.kbd,(int)i);
    }
    memcpy(held,keys,sizeof held);
    zx_joystick(&machine,((pad&R8_RIGHT)?1:0)|((pad&R8_LEFT)?2:0)|((pad&R8_DOWN)?4:0)|
        ((pad&R8_UP)?8:0)|((pad&R8_A)?16:0));
}
R8_FRAME_ATTR void r8_frame(void) {
    if(!loaded)return;
    for(unsigned i=0;i<19;i++){zx_exec(&machine,1000);retro8_service();}
    zx_exec(&machine,968);retro8_service();
}
#if R8_ZX_PACKED_RENDER
#include "retro8_zx_render.h"
#endif
void r8_render(uint16_t *dest) {
    chips_display_info_t info=zx_display_info(&machine);
    const uint32_t *palette=info.palette.ptr;uint16_t colors[16];
    for(unsigned i=0;i<16;i++){uint32_t c=palette[i];colors[i]=((c&255)>>3)<<10|(((c>>8)&255)>>3)<<5|(((c>>16)&255)>>3);}
#if R8_ZX_PACKED_RENDER
    r8_zx_packed_render(dest,machine.fb,colors);
#else
    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++)dest[y*320+x]=colors[machine.fb[(y+8)*512+x]&15];
#endif
}
const char *r8_error(void){return error_text;}
uint32_t r8_clock(void){return 3500000;}
uint32_t r8_frame_cycles(void){return 69888;}
uint16_t r8_pc(void){return machine.cpu.pc;}
uint8_t r8_peek(uint16_t address){return mem_rd(&machine.mem,address);}
