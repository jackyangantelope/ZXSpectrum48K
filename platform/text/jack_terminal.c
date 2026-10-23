

/* SPDX-License-Identifier: BSD-3-Clause */
#include "jack_terminal.h"

bool jack_terminal_init(jack_terminal *terminal, uint16_t width, uint16_t height,
                        jack_terminal_cell *cells, size_t cell_capacity) {
    if(terminal == NULL || cells == NULL || width == 0 || height == 0 ||
       (size_t)width * height > cell_capacity) {
        return false;
    }
    terminal->width = width;
    terminal->height = height;
    terminal->cells = cells;
    terminal->cell_capacity = cell_capacity;
    terminal->key_head = 0;
    terminal->key_tail = 0;
    return true;
}

void jack_terminal_clear(jack_terminal *terminal, uint8_t glyph, uint8_t foreground,
                         uint8_t background) {
    const size_t count = (size_t)terminal->width * terminal->height;
    for(size_t i = 0; i < count; ++i) {
        terminal->cells[i].glyph = glyph;
        terminal->cells[i].foreground = foreground;
        terminal->cells[i].background = background;
        terminal->cells[i].flags = 0;
    }
}

bool jack_terminal_put(jack_terminal *terminal, uint16_t x, uint16_t y, uint8_t glyph,
                       uint8_t foreground, uint8_t background) {
    if(x >= terminal->width || y >= terminal->height) {
        return false;
    }
    jack_terminal_cell *cell = &terminal->cells[(size_t)y * terminal->width + x];
    cell->glyph = glyph;
    cell->foreground = foreground;
    cell->background = background;
    cell->flags = 0;
    return true;
}

size_t jack_terminal_write(jack_terminal *terminal, uint16_t x, uint16_t y,
                           const char *text, uint8_t foreground, uint8_t background) {
    size_t written = 0;
    while(text[written] != '\0' && x + written < terminal->width) {
        jack_terminal_put(terminal, (uint16_t)(x + written), y, (uint8_t)text[written],
                          foreground, background);
        ++written;
    }
    return written;
}

bool jack_terminal_box(jack_terminal *terminal, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height, uint8_t foreground,
                       uint8_t background) {
    if(width < 2 || height < 2 || x + width > terminal->width ||
       y + height > terminal->height) {
        return false;
    }
    for(uint16_t column = 0; column < width; ++column) {
        jack_terminal_put(terminal, x + column, y, '-', foreground, background);
        jack_terminal_put(terminal, x + column, y + height - 1, '-', foreground, background);
    }
    for(uint16_t row = 0; row < height; ++row) {
        jack_terminal_put(terminal, x, y + row, '|', foreground, background);
        jack_terminal_put(terminal, x + width - 1, y + row, '|', foreground, background);
    }
    jack_terminal_put(terminal, x, y, '+', foreground, background);
    jack_terminal_put(terminal, x + width - 1, y, '+', foreground, background);
    jack_terminal_put(terminal, x, y + height - 1, '+', foreground, background);
    jack_terminal_put(terminal, x + width - 1, y + height - 1, '+', foreground, background);
    return true;
}

bool jack_terminal_push_key(jack_terminal *terminal, uint16_t key) {
    const uint8_t next = (uint8_t)((terminal->key_head + 1u) % JACK_TERMINAL_KEY_CAPACITY);
    if(next == terminal->key_tail) {
        return false;
    }
    terminal->keys[terminal->key_head] = key;
    terminal->key_head = next;
    return true;
}

bool jack_terminal_pop_key(jack_terminal *terminal, uint16_t *key) {
    if(terminal->key_tail == terminal->key_head || key == NULL) {
        return false;
    }
    *key = terminal->keys[terminal->key_tail];
    terminal->key_tail = (uint8_t)((terminal->key_tail + 1u) % JACK_TERMINAL_KEY_CAPACITY);
    return true;
}

uint32_t jack_terminal_hash(const jack_terminal *terminal) {
    uint32_t hash = 2166136261u;
    const size_t count = (size_t)terminal->width * terminal->height;
    for(size_t i = 0; i < count; ++i) {
        const uint8_t *bytes = (const uint8_t *)&terminal->cells[i];
        for(size_t byte = 0; byte < sizeof(terminal->cells[i]); ++byte) {
            hash ^= bytes[byte];
            hash *= 16777619u;
        }
    }
    return hash;
}
