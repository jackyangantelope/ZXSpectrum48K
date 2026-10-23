

/* SPDX-License-Identifier: BSD-3-Clause */
#include "jack_input.h"

void jack_input_init(jack_input_queue *queue) {
    queue->head = 0;
    queue->tail = 0;
    queue->dropped = 0;
}

bool jack_input_push(jack_input_queue *queue, jack_input_event event) {
    const uint8_t next = (uint8_t)((queue->head + 1u) % JACK_INPUT_CAPACITY);
    if(next == queue->tail) {
        queue->dropped++;
        return false;
    }
    queue->events[queue->head] = event;
    queue->head = next;
    return true;
}

bool jack_input_pop(jack_input_queue *queue, jack_input_event *event) {
    if(queue->head == queue->tail || event == 0) {
        return false;
    }
    *event = queue->events[queue->tail];
    queue->tail = (uint8_t)((queue->tail + 1u) % JACK_INPUT_CAPACITY);
    return true;
}

bool jack_input_translate_hid(uint8_t usage, uint8_t modifier, uint16_t *key) {
    if(key == 0) {
        return false;
    }
    const bool shift = (modifier & 0x22u) != 0;
    if(usage >= 0x04u && usage <= 0x1du) {
        *key = (uint16_t)((shift ? 'A' : 'a') + usage - 0x04u);
        return true;
    }
    if(usage >= 0x1eu && usage <= 0x26u) {
        static const char shifted[] = "!@#$%^&*(";
        *key = shift ? (uint16_t)shifted[usage - 0x1eu]
                     : (uint16_t)('1' + usage - 0x1eu);
        return true;
    }
    if(usage == 0x27u) {
        *key = shift ? ')' : '0';
        return true;
    }
    switch(usage) {
    case 0x28: *key = JACK_KEY_ENTER; return true;
    case 0x29: *key = JACK_KEY_ESCAPE; return true;
    case 0x2a: *key = JACK_KEY_BACKSPACE; return true;
    case 0x2b: *key = JACK_KEY_TAB; return true;
    case 0x2c: *key = ' '; return true;
    case 0x2d: *key = shift ? '_' : '-'; return true;
    case 0x2e: *key = shift ? '+' : '='; return true;
    case 0x2f: *key = shift ? '{' : '['; return true;
    case 0x30: *key = shift ? '}' : ']'; return true;
    case 0x31: *key = shift ? '|' : '\\'; return true;
    case 0x33: *key = shift ? ':' : ';'; return true;
    case 0x34: *key = shift ? '"' : '\''; return true;
    case 0x35: *key = shift ? '~' : '`'; return true;
    case 0x36: *key = shift ? '<' : ','; return true;
    case 0x37: *key = shift ? '>' : '.'; return true;
    case 0x38: *key = shift ? '?' : '/'; return true;
    case 0x4a: *key = JACK_KEY_HOME; return true;
    case 0x4b: *key = JACK_KEY_PAGE_UP; return true;
    case 0x4c: *key = JACK_KEY_DELETE; return true;
    case 0x4d: *key = JACK_KEY_END; return true;
    case 0x4e: *key = JACK_KEY_PAGE_DOWN; return true;
    case 0x4f: *key = JACK_KEY_RIGHT; return true;
    case 0x50: *key = JACK_KEY_LEFT; return true;
    case 0x51: *key = JACK_KEY_DOWN; return true;
    case 0x52: *key = JACK_KEY_UP; return true;
    default: return false;
    }
}
