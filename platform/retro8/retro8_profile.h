/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef JACK_RETRO8_PROFILE_H
#define JACK_RETRO8_PROFILE_H
#if R8_PROFILE
#include <stdarg.h>
#include "hardware/uart.h"
#ifndef R8_PROFILE_UART_WRITE

#define R8_PROFILE_UART_WRITE(c) (uart_get_hw(uart_get_instance(PICO_DEFAULT_UART))->dr=(uint8_t)(c))
#endif
static struct {
    uint64_t start_us, service_us, usb_us, pad_us, audio_us;
    uint64_t emu_us, render_us, work_us;
    uint32_t completed, shown, service_calls;
} r8_profile;
#ifndef R8_PROFILE_TX_CAPACITY
#define R8_PROFILE_TX_CAPACITY 1536
#endif
static char r8_tx[R8_PROFILE_TX_CAPACITY],r8_format[768];
static size_t r8_tx_pos,r8_tx_size;
static uint32_t r8_tx_drops;
#if R8_BEEPER_STEADY
static r8_beeper_stats r8_beeper_start;
#endif
static void r8_profile_pump(void) {

    for(unsigned n=0;n<16&&r8_tx_pos<r8_tx_size;n++) {
        if(!uart_is_writable(uart_get_instance(PICO_DEFAULT_UART)))break;
        R8_PROFILE_UART_WRITE(r8_tx[r8_tx_pos++]);
    }
    if(r8_tx_pos==r8_tx_size)r8_tx_pos=r8_tx_size=0;
}
static void r8_profile_printf(const char *format,...) {
    va_list args;va_start(args,format);
    int n=vsnprintf(r8_format,sizeof r8_format,format,args);va_end(args);
    size_t pending=r8_tx_size-r8_tx_pos;
    if(n<0||(size_t)n>=sizeof r8_format||(size_t)n>sizeof r8_tx-pending){r8_tx_drops++;return;}
    memmove(r8_tx,r8_tx+r8_tx_pos,pending);
    memcpy(r8_tx+pending,r8_format,(size_t)n);
    r8_tx_pos=0;r8_tx_size=pending+(size_t)n;
    r8_profile_pump();
}
static void r8_profile_reset(uint64_t now) {
    memset(&r8_profile,0,sizeof r8_profile);r8_profile.start_us=now;
#if R8_BEEPER_STEADY
    r8_beeper_get(&r8_beeper_start);
#endif
#if R8_ZX_SPLIT_PROFILE
    r8_zx_split_reset();
#endif
}
static unsigned long r8_profile_average(uint64_t total,uint32_t n) {
    return n?(unsigned long)(total/n):0;
}
static void r8_profile_report(uint32_t frames,uint32_t max_us,uint32_t slow,
                              unsigned count,uint32_t under,uint32_t over,
                              uint32_t resync,unsigned pc) {
    uint64_t now=time_us_64(),elapsed=now-r8_profile.start_us;
    r8_profile_printf("R8_PERF frames=%lu max_us=%lu slow=%lu audio_q=%u under=%lu over=%lu resync=%lu pc=%04x\n"
        "R8_PROFILE variant=" R8_VARIANT " interval_us=%llu frames=%lu fps_milli=%lu shown=%lu "
        "emu_avg_us=%lu render_avg_us=%lu work_avg_us=%lu svc_us=%llu usb_us=%llu pad_us=%llu audio_us=%llu "
        "svc_calls=%lu tx_drop=%lu\n",
        (unsigned long)frames,(unsigned long)max_us,(unsigned long)slow,count,
        (unsigned long)under,(unsigned long)over,(unsigned long)resync,pc,
        (unsigned long long)elapsed,(unsigned long)r8_profile.completed,
        elapsed?(unsigned long)((uint64_t)r8_profile.completed*1000000000ull/elapsed):0,
        (unsigned long)r8_profile.shown,
        r8_profile_average(r8_profile.emu_us,r8_profile.completed),
        r8_profile_average(r8_profile.render_us,r8_profile.shown),
        r8_profile_average(r8_profile.work_us,r8_profile.completed),
        (unsigned long long)r8_profile.service_us,(unsigned long long)r8_profile.usb_us,
        (unsigned long long)r8_profile.pad_us,(unsigned long long)r8_profile.audio_us,
        (unsigned long)r8_profile.service_calls,(unsigned long)r8_tx_drops);
#if R8_BEEPER_STEADY
    r8_beeper_stats beep;r8_beeper_get(&beep);
    uint32_t fast=beep.fast_ticks-r8_beeper_start.fast_ticks;
    uint32_t slow_ticks=beep.slow_ticks-r8_beeper_start.slow_ticks;
    uint64_t total=(uint64_t)fast+slow_ticks;
    r8_profile_printf("R8_BEEPER variant=" R8_VARIANT " frames=%lu fast_ticks=%lu slow_ticks=%lu "
        "entries=%lu fast_permille=%lu valid=%u\n",(unsigned long)r8_profile.completed,
        (unsigned long)fast,(unsigned long)slow_ticks,(unsigned long)(beep.entries-r8_beeper_start.entries),
        total?(unsigned long)((uint64_t)fast*1000/total):0,
        (unsigned)(total==(uint64_t)r8_profile.completed*r8_frame_cycles()));
#endif
#if R8_ZX_SPLIT_PROFILE
    r8_zx_split_stats zx;r8_zx_split_get(&zx);
    bool invalid=zx.ula_us>r8_profile.emu_us;
    r8_profile_printf("R8_ZX_SPLIT variant=" R8_VARIANT " frames=%lu ula_avg_us=%lu "
        "active_avg_us=%lu border_avg_us=%lu blank_avg_us=%lu other_avg_us=%lu "
        "ula_permille=%lu calls=%lu active_rows=%lu border_rows=%lu blank_rows=%lu "
        "max_line_us=%lu invalid=%u\n",
        (unsigned long)r8_profile.completed,
        r8_profile_average(zx.ula_us,r8_profile.completed),
        r8_profile_average(zx.active_us,r8_profile.completed),
        r8_profile_average(zx.border_us,r8_profile.completed),
        r8_profile_average(zx.blank_us,r8_profile.completed),
        r8_profile_average(invalid?0:r8_profile.emu_us-zx.ula_us,r8_profile.completed),
        r8_profile.emu_us?(unsigned long)(zx.ula_us*1000/r8_profile.emu_us):0,
        (unsigned long)zx.calls,(unsigned long)zx.active_rows,
        (unsigned long)zx.border_rows,(unsigned long)zx.blank_rows,
        (unsigned long)zx.max_line_us,(unsigned)invalid);
#endif

    r8_profile_reset(now);
}
#else
#define r8_profile_printf printf
#endif
#endif
