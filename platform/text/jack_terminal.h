

/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef JACK_YELLOW_TERMINAL_H
#define JACK_YELLOW_TERMINAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define JACK_TERMINAL_KEY_CAPACITY 16u

typedef struct {
    uint8_t glyph;
    uint8_t foreground;
    uint8_t background;
    uint8_t flags;
} jack_terminal_cell;

typedef struct {
    uint16_t width;
    uint16_t height;
    jack_terminal_cell *cells;
    size_t cell_capacity;
    uint16_t keys[JACK_TERMINAL_KEY_CAPACITY];
    uint8_t key_head;
    uint8_t key_tail;
} jack_terminal;

bool jack_terminal_init(jack_terminal *terminal, uint16_t width, uint16_t height,
                        jack_terminal_cell *cells, size_t cell_capacity);
void jack_terminal_clear(jack_terminal *terminal, uint8_t glyph, uint8_t foreground,
                         uint8_t background);
bool jack_terminal_put(jack_terminal *terminal, uint16_t x, uint16_t y, uint8_t glyph,
                       uint8_t foreground, uint8_t background);
size_t jack_terminal_write(jack_terminal *terminal, uint16_t x, uint16_t y,
                           const char *text, uint8_t foreground, uint8_t background);
bool jack_terminal_box(jack_terminal *terminal, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, uint8_t foreground,
                       uint8_t background);
bool jack_terminal_push_key(jack_terminal *terminal, uint16_t key);
bool jack_terminal_pop_key(jack_terminal *terminal, uint16_t *key);
uint32_t jack_terminal_hash(const jack_terminal *terminal);

#endif
