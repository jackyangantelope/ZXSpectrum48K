

/* SPDX-License-Identifier: MIT */
#ifndef JACK_YELLOW_TUSB_CONFIG_H
#define JACK_YELLOW_TUSB_CONFIG_H
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined by the Pico SDK
#endif
#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif
#define CFG_TUSB_DEBUG 0
#define CFG_TUD_ENABLED 0
#define CFG_TUH_ENABLED 1
#define CFG_TUH_RPI_PIO_USB 1
#define BOARD_TUH_RHPORT 1
#define BOARD_TUH_MAX_SPEED OPT_MODE_DEFAULT_SPEED
#define CFG_TUH_MAX_SPEED BOARD_TUH_MAX_SPEED
#define CFG_TUD_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUH_MEM_ALIGN __attribute__((aligned(4)))
#define CFG_TUH_ENUMERATION_BUFSIZE 256
#define CFG_TUH_HUB 1
#define CFG_TUH_DEVICE_MAX 4
#define CFG_TUH_HID (3 * CFG_TUH_DEVICE_MAX)
#define CFG_TUH_HID_EPIN_BUFSIZE 64
#define CFG_TUH_HID_EPOUT_BUFSIZE 64
#endif
