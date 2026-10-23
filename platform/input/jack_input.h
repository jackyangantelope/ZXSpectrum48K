

/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef JACK_YELLOW_INPUT_H
#define JACK_YELLOW_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#define JACK_INPUT_CAPACITY 32u

typedef enum {
    JACK_INPUT_NONE = 0,
    JACK_INPUT_KEY_DOWN,
    JACK_INPUT_KEY_UP,
    JACK_INPUT_PAD_DOWN,
    JACK_INPUT_PAD_UP,
    JACK_INPUT_TEXT,
    JACK_INPUT_MOUSE_MOVE,
    JACK_INPUT_MOUSE_BUTTON,
    JACK_INPUT_PAD_AXIS
} jack_input_type;

typedef struct {
    jack_input_type type;
    uint8_t device;
    uint16_t code;
    int16_t value;
} jack_input_event;

enum {
    JACK_PAD_A=1u<<0, JACK_PAD_B=1u<<1, JACK_PAD_X=1u<<2, JACK_PAD_Y=1u<<3,
    JACK_PAD_SELECT=1u<<4, JACK_PAD_START=1u<<5, JACK_PAD_UP=1u<<6,
    JACK_PAD_DOWN=1u<<7, JACK_PAD_LEFT=1u<<8, JACK_PAD_RIGHT=1u<<9,
    JACK_PAD_L=1u<<10, JACK_PAD_R=1u<<11
};

typedef struct {
    jack_input_event events[JACK_INPUT_CAPACITY];
    uint8_t head;
    uint8_t tail;
    uint32_t dropped;
} jack_input_queue;

enum {
    JACK_KEY_ENTER = 0x100,
    JACK_KEY_ESCAPE,
    JACK_KEY_BACKSPACE,
    JACK_KEY_TAB,
    JACK_KEY_LEFT,
    JACK_KEY_RIGHT,
    JACK_KEY_UP,
    JACK_KEY_DOWN,
    JACK_KEY_HOME,
    JACK_KEY_END,
    JACK_KEY_PAGE_UP,
    JACK_KEY_PAGE_DOWN,
    JACK_KEY_DELETE
};

void jack_input_init(jack_input_queue *queue);
bool jack_input_push(jack_input_queue *queue, jack_input_event event);
bool jack_input_pop(jack_input_queue *queue, jack_input_event *event);
bool jack_input_translate_hid(uint8_t usage, uint8_t modifier, uint16_t *key);

#endif
