

/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef JACK_YELLOW_TINYUSB_INPUT_H
#define JACK_YELLOW_TINYUSB_INPUT_H
#include <stdbool.h>
#include "jack_input_devices.h"
bool jack_tinyusb_input_init(jack_input_devices *devices);
void jack_tinyusb_input_poll(void);
int jack_tinyusb_second_port_result(void);
#endif
