

/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef JACK_YELLOW_INPUT_DEVICES_H
#define JACK_YELLOW_INPUT_DEVICES_H
#include <stdbool.h>
#include <stdint.h>
#include "jack_input.h"
#include "jack_terminal.h"
typedef struct {
    jack_input_queue *queue;
    uint16_t pads[2];
    uint8_t keyboard_modifier, keyboard_keys[6], mouse_buttons;
    int32_t mouse_x, mouse_y, mouse_wheel;
} jack_input_devices;
typedef struct { bool active, shift; uint8_t x, y; } jack_osk;
#ifdef __cplusplus
extern "C" {
#endif
bool jack_input_devices_init(jack_input_devices *devices, jack_input_queue *queue);
bool jack_input_set_pad(jack_input_devices *devices, uint8_t pad, uint16_t buttons);
bool jack_input_keyboard_report(jack_input_devices *devices, uint8_t modifier, const uint8_t keys[6]);
bool jack_input_mouse_report(jack_input_devices *devices, uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel);
void jack_osk_init(jack_osk *osk);
bool jack_osk_handle(jack_osk *osk, uint16_t key, jack_input_queue *queue);
bool jack_osk_render(const jack_osk *osk, jack_terminal *terminal, uint16_t x, uint16_t y);
#ifdef __cplusplus
}
#endif
#endif
