/* SPDX-License-Identifier: BSD-3-Clause */

typedef struct {
    uint32_t raw, sum;
    unsigned warm;
    bool ready;
} r8_halt_filter;

static uint32_t _r8_float_bits(float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

/* The filter sum depends on floating-point operation order. */
static void _r8_halt_filter_advance(beeper_t* b, r8_halt_filter* f, unsigned n) {
    const float raw = (float)b->state * b->volume * b->base_volume;
    const uint32_t bits = _r8_float_bits(raw);
    if (bits != f->raw || _r8_float_bits(b->dcadj_sum) != f->sum) {
        f->warm = 0;
        f->ready = false;
        f->raw = bits;
    }
    unsigned left = n;
    while (left && !f->ready) {
        _beeper_dcadjust(b, raw);
        left--;
        if (f->warm < BEEPER_DCADJ_BUFLEN) f->warm++;
        if (f->warm == BEEPER_DCADJ_BUFLEN &&
            (bits & 0x7f800000u) != 0x7f800000u &&
            (_r8_float_bits(b->dcadj_sum) & 0x7f800000u) != 0x7f800000u) {
            float next = b->dcadj_sum - raw;
            next += raw;
            f->ready = _r8_float_bits(next) == _r8_float_bits(b->dcadj_sum);
        }
    }
    b->dcadj_pos = (b->dcadj_pos + left) & (BEEPER_DCADJ_BUFLEN - 1);
    b->counter -= (int)(n * BEEPER_FIXEDPOINT_SCALE);
    f->sum = _r8_float_bits(b->dcadj_sum);
}

/* Batch only canonical HALT fetches without pending interrupts or external pins. */
static unsigned _r8_halt_span(zx_t* s, uint64_t pins, unsigned left) {
    const z80_t* c = &s->cpu;
    uint64_t expected = Z80_HALT | Z80_M1 | Z80_MREQ | Z80_RD |
        (uint16_t)(c->pc - 1) | (UINT64_C(0x76) << 16);
    if (pins != expected || c->pins != pins || c->int_bits ||
        c->step != Z80_M1_T2 || c->opcode != 0x76 ||
        c->prefix_active || c->hlx_idx || c->addr != c->hl ||
        mem_rd(&s->mem, (uint16_t)(c->pc - 1)) != 0x76 ||
        s->scanline_counter <= 0 || s->beeper.counter <= 0) return 0;
    unsigned n = (unsigned)(s->scanline_counter - 1);
    unsigned audio = (unsigned)(s->beeper.counter - 1) / BEEPER_FIXEDPOINT_SCALE;
    if (n > audio) n = audio;
    if (n > left) n = left;
    return n & ~3u;
}

typedef struct { uint64_t pins; uint32_t ticks; } r8_halt_result;
static r8_halt_result _r8_zx_halt_run(zx_t* s, uint64_t pins, uint32_t count) {
    uint32_t tick = 0;
    r8_halt_filter filter = {0};
    while (tick < count && (pins & Z80_HALT)) {
        unsigned n = _r8_halt_span(s, pins, count - tick);
        if (n) {
            _r8_halt_filter_advance(&s->beeper, &filter, n);
            s->cpu.r = (s->cpu.r & 0x80) | ((s->cpu.r + n / 4) & 0x7f);
            s->scanline_counter -= (int)n;
            s->tick_count += n;
            tick += n;
        } else {
            pins = _zx_tick(s, pins);
            tick++;
        }
    }
    return (r8_halt_result){pins, tick};
}
