/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef JACK_RETRO8_H
#define JACK_RETRO8_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef void (*r8_audio_fn)(int16_t left, int16_t right);
typedef void (*r8_service_fn)(void);
typedef struct { uint16_t pads[2]; uint8_t keys[6], modifiers; } r8_input;
enum { R8_A=1, R8_B=2, R8_X=4, R8_Y=8, R8_SELECT=16, R8_START=32,
       R8_UP=64, R8_DOWN=128, R8_LEFT=256, R8_RIGHT=512 };
bool r8_init(void *bulk, size_t capacity, r8_audio_fn audio, r8_service_fn service);
bool r8_load(const uint8_t *game, size_t size, const uint8_t *bios, size_t bios_size, const char *extension);
void r8_controls(const r8_input *state);
void r8_frame(void);
void r8_render(uint16_t *rgb555_320x240);
const char *r8_error(void);
uint32_t r8_clock(void);
uint32_t r8_frame_cycles(void);
uint16_t r8_pc(void);
uint8_t r8_peek(uint16_t address);
#if R8_COLECO_HALT_BATCH
void r8_coleco_halt_get(uint64_t *ticks,uint32_t *spans);
#endif
void retro8_service(void);
#if R8_BEEPER_STEADY
typedef struct { uint32_t fast_ticks,slow_ticks,entries; } r8_beeper_stats;
void r8_beeper_get(r8_beeper_stats *out);
#endif
#if R8_ZX_SPLIT_PROFILE

typedef struct {
    uint64_t ula_us,active_us,border_us,blank_us;
    uint32_t calls,active_rows,border_rows,blank_rows,max_line_us;
} r8_zx_split_stats;
uint32_t r8_zx_profile_now(void);
void r8_zx_split_get(r8_zx_split_stats *out);
void r8_zx_split_reset(void);
#endif
#endif
