

/* SPDX-License-Identifier: BSD-3-Clause */
#include "jack_tinyusb_input.h"
#include "hardware/dma.h"
#include "pio_usb.h"
#include "tusb.h"

static jack_input_devices *input_devices;
static int second_port_result = -999;

#if JACK_TINYUSB_KEYBOARD_RECOVERY
static uint8_t keyboard_owner_address, keyboard_owner_instance;
#endif

bool jack_tinyusb_input_init(jack_input_devices *devices) {
    if(devices == NULL) return false;
    input_devices = devices;
#if JACK_TINYUSB_KEYBOARD_RECOVERY
    keyboard_owner_address = keyboard_owner_instance = 0;
#endif
    pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
    config.pio_rx_num = 2;
    config.pio_tx_num = 2;
    config.pin_dp = 28;
    const int tx_channel = dma_claim_unused_channel(false);
    if(tx_channel < 0) return false;
    config.tx_ch = (uint8_t)tx_channel;
    if(!tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config)) {
        dma_channel_unclaim((uint)tx_channel);
        return false;
    }
    const tusb_rhport_init_t host = {.role=TUSB_ROLE_HOST,.speed=TUSB_SPEED_AUTO};

    dma_channel_unclaim((uint)tx_channel);
    if(!tusb_init(BOARD_TUH_RHPORT,&host)) return false;
    second_port_result = pio_usb_host_add_port(26, PIO_USB_PINOUT_DPDM);
    return true;
}

void jack_tinyusb_input_poll(void) { tuh_task(); }
int jack_tinyusb_second_port_result(void) { return second_port_result; }

void tuh_hid_mount_cb(uint8_t dev_addr,uint8_t instance,const uint8_t *desc,uint16_t length) {
    (void)desc;(void)length;
    (void)tuh_hid_receive_report(dev_addr,instance);
}
void tuh_hid_umount_cb(uint8_t dev_addr,uint8_t instance) {
#if JACK_TINYUSB_KEYBOARD_RECOVERY
    if (input_devices && keyboard_owner_address == dev_addr &&
        keyboard_owner_instance == instance) {
        const uint8_t released[6] = {0};
        (void)jack_input_keyboard_report(input_devices, 0, released);
        keyboard_owner_address = keyboard_owner_instance = 0;
    }
#else
    (void)dev_addr; (void)instance;
#endif
}
void tuh_hid_report_received_cb(uint8_t dev_addr,uint8_t instance,const uint8_t *report,uint16_t length) {
    if(input_devices != NULL) {
        const uint8_t protocol=tuh_hid_interface_protocol(dev_addr,instance);
        if(protocol==HID_ITF_PROTOCOL_KEYBOARD && length>=sizeof(hid_keyboard_report_t)) {
#if JACK_TINYUSB_KEYBOARD_RECOVERY
            keyboard_owner_address = dev_addr;
            keyboard_owner_instance = instance;
#endif
            const hid_keyboard_report_t *keyboard=(const hid_keyboard_report_t *)report;
            (void)jack_input_keyboard_report(input_devices,keyboard->modifier,keyboard->keycode);
        } else if(protocol==HID_ITF_PROTOCOL_MOUSE && length>=3) {

            (void)jack_input_mouse_report(input_devices,report[0],(int8_t)report[1],
                                          (int8_t)report[2],length>=4?(int8_t)report[3]:0);
        }
    }
    (void)tuh_hid_receive_report(dev_addr,instance);
}
