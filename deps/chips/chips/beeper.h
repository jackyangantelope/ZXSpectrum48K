#pragma once
/*
    beeper.h    -- simple square-wave beeper

    TODO: docs

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
*/
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// error-accumulation precision boost
#define BEEPER_FIXEDPOINT_SCALE (16)
// DC adjust buffer size
#define BEEPER_DCADJ_BUFLEN (128)

// initialization parameters
typedef struct {
    int tick_hz;
    int sound_hz;
    float base_volume;
} beeper_desc_t;

// beeper state
typedef struct {
    int state;
    int period;
    int counter;
    float base_volume;
    float volume;
    float sample;
    float dcadj_sum;
    uint32_t dcadj_pos;
    float dcadj_buf[BEEPER_DCADJ_BUFLEN];
#if R8_BEEPER_STEADY
    /* Port opt-in cache. All upstream filter fields retain their offsets.
       No game-name or mute shortcut: only an exact floating-point fixed point
       after a complete constant-input filter window may bypass the work. */
    struct {
        uint32_t state,volume,base,raw,sum,run;
        uint32_t fast_ticks,slow_ticks,entries;
        bool ready;
    } steady;
#endif
} beeper_t;

// initialize beeper instance
void beeper_init(beeper_t* beeper, const beeper_desc_t* desc);
// reset the beeper instance
void beeper_reset(beeper_t* beeper);
// set current on/off state
static inline void beeper_set(beeper_t* beeper, bool state) {
    beeper->state = state ? 1 : 0;
}
// toggle current state (on->off or off->on)
static inline void beeper_toggle(beeper_t* beeper) {
    beeper->state = !beeper->state;
}
// set current volume 0.0 to 1.0
static inline void beeper_set_volume(beeper_t* beeper, float vol) {
    beeper->volume = vol;
}
// tick the beeper, return true if a new sample is ready
bool beeper_tick(beeper_t* beeper);

#ifdef __cplusplus
} /* extern "C" */
#endif
/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
    #include <assert.h>
    #define CHIPS_ASSERT(c) assert(c)
#endif

void beeper_init(beeper_t* b, const beeper_desc_t* desc) {
    CHIPS_ASSERT(b && desc);
    CHIPS_ASSERT((desc->tick_hz > 0) && (desc->sound_hz > 0));
    *b = (beeper_t){
        .period = (desc->tick_hz * BEEPER_FIXEDPOINT_SCALE) / desc->sound_hz,
        .counter = b->period,
        .base_volume = desc->base_volume,
        .volume = 1.0f,
    };
}

void beeper_reset(beeper_t* b) {
    CHIPS_ASSERT(b);
    b->state = 0;
    b->counter = b->period;
    b->sample = 0;
#if R8_BEEPER_STEADY
    /* Reset deliberately retains the upstream DC history, sum and position. */
    memset(&b->steady,0,sizeof b->steady);
#endif
}

/* DC adjustment filter from StSound, this moves an "offcenter"
   signal back to the zero-line (e.g. the volume-level output
   from the chip simulation which is >0.0 gets converted to
   a +/- sample value)
*/
static void _beeper_dcadjust(beeper_t* bp, float s) {
    bp->dcadj_sum -= bp->dcadj_buf[bp->dcadj_pos];
    bp->dcadj_sum += s;
    bp->dcadj_buf[bp->dcadj_pos] = s;
    bp->dcadj_pos = (bp->dcadj_pos + 1) & (BEEPER_DCADJ_BUFLEN-1);
}

#if R8_BEEPER_STEADY
static inline uint32_t _beeper_bits(float value) {
    uint32_t bits;memcpy(&bits,&value,sizeof bits);return bits;
}
static inline bool _beeper_finite(uint32_t bits) {
    return (bits&0x7f800000u)!=0x7f800000u;
}
static inline void _beeper_steady_dcadjust(beeper_t* bp) {
    const uint32_t state=(uint32_t)bp->state;
    const uint32_t volume=_beeper_bits(bp->volume),base=_beeper_bits(bp->base_volume);
    const bool same=bp->steady.run&&state==bp->steady.state&&volume==bp->steady.volume&&base==bp->steady.base;
    if(same&&bp->steady.ready&&_beeper_bits(bp->dcadj_sum)==bp->steady.sum) {
        /* Every slot already contains the identical raw value. The original
           subtraction/addition has been checked to leave sum bit-identical. */
        bp->dcadj_pos=(bp->dcadj_pos+1)&(BEEPER_DCADJ_BUFLEN-1);
        bp->steady.fast_ticks++;
        return;
    }
    const float raw=(float)bp->state*bp->volume*bp->base_volume;
    const uint32_t raw_bits=_beeper_bits(raw);
    const bool was_ready=bp->steady.ready;
    bp->steady.ready=false;
    _beeper_dcadjust(bp,raw);
    bp->steady.slow_ticks++;
    if(!same||raw_bits!=bp->steady.raw) {
        bp->steady.state=state;bp->steady.volume=volume;bp->steady.base=base;
        bp->steady.raw=raw_bits;bp->steady.run=1;
    }else if(bp->steady.run<BEEPER_DCADJ_BUFLEN)bp->steady.run++;
    if(bp->steady.run==BEEPER_DCADJ_BUFLEN&&_beeper_finite(raw_bits)&&_beeper_finite(_beeper_bits(bp->dcadj_sum))) {
        /* Never replace repeated floating arithmetic with raw*window_size:
           that would change rounding and the original waveform. */
        float next=bp->dcadj_sum-raw;next+=raw;
        if(_beeper_bits(next)==_beeper_bits(bp->dcadj_sum)) {
            bp->steady.sum=_beeper_bits(bp->dcadj_sum);bp->steady.ready=true;
            if(!was_ready)bp->steady.entries++;
        }
    }
}
#endif

bool beeper_tick(beeper_t* bp) {
#if R8_BEEPER_STEADY
    _beeper_steady_dcadjust(bp);
#else
    _beeper_dcadjust(bp, (float)bp->state * bp->volume * bp->base_volume);
#endif
    /* generate a new sample? */
    bp->counter -= BEEPER_FIXEDPOINT_SCALE;
    if (bp->counter <= 0) {
        bp->counter += bp->period;
        bp->sample = bp->dcadj_sum / BEEPER_DCADJ_BUFLEN;
        return true;
    }
    return false;
}


#endif /* CHIPS_IMPL */
