cmake_minimum_required(VERSION 3.20)
set(PICO_BOARD adafruit_fruit_jam CACHE STRING "Adafruit Fruit Jam" FORCE)
set(PICO_PSRAM_CS_PIN 47 CACHE STRING "Fruit Jam PSRAM CS" FORCE)
set(PICO_PSRAM_SIZE_BYTES 0x800000 CACHE STRING "Fruit Jam PSRAM size" FORCE)
set(PICO_DEFAULT_UART 0 CACHE STRING "Diagnostic UART" FORCE)
set(PICO_DEFAULT_UART_TX_PIN 44 CACHE STRING "Diagnostic UART TX" FORCE)
set(PICO_DEFAULT_UART_RX_PIN 45 CACHE STRING "Diagnostic UART RX" FORCE)
if(NOT DEFINED PICO_SDK_PATH OR NOT DEFINED PICO_PIO_USB_PATH)
  message(FATAL_ERROR "Set PICO_SDK_PATH and PICO_PIO_USB_PATH")
endif()
set(ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(SHARED "${ROOT}/deps/pico_shared")
set(HDMI "${SHARED}/drivers/pico_hdmi")
set(FATFS "${SHARED}/drivers/pico_fatfs")
include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
project(ZXSpectrum48K C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()
set(SAVED_TINYUSB_TOP "${TOP}")
set(TOP "${PICO_SDK_PATH}/lib/tinyusb")
check_and_add_pico_pio_usb_support()
set(TOP "${SAVED_TINYUSB_TOP}")
add_subdirectory("${FATFS}" "${CMAKE_BINARY_DIR}/pico_fatfs")

add_library(zx48_core STATIC "${ROOT}/platform/retro8/zx_runtime.c")
target_include_directories(zx48_core PUBLIC "${ROOT}/platform/retro8" PRIVATE "${ROOT}/deps/chips")
target_compile_options(zx48_core PRIVATE -O3 -funsigned-char -ffunction-sections -fdata-sections)
target_compile_definitions(zx48_core PRIVATE R8_SKIP_48K_AY=1 R8_SRAM_HOT=0
  R8_ZX_SPLIT_PROFILE=0 R8_BEEPER_STEADY=0 R8_FUSED_TICK=1 R8_Z80_INLINE=1
  R8_HALT_BATCH=1 R8_ZX_PACKED_RENDER=1)
target_link_libraries(zx48_core PUBLIC pico_stdlib)

add_library(zx48_hdmi STATIC "${HDMI}/hstx.c" "${HDMI}/video_output.c"
  "${HDMI}/hstx_data_island_queue.c" "${HDMI}/hstx_packet.c")
target_include_directories(zx48_hdmi PUBLIC "${HDMI}")
target_compile_definitions(zx48_hdmi PRIVATE GPIOHSTXD0=15 GPIOHSTXD1=17 GPIOHSTXD2=19
  GPIOHSTXCK=13 GPIOHSTXINVERTED=1 JACK_HSTX_126MHZ=1 JACK_HSTX_DOUBLE_BUFFER=1)
target_compile_options(zx48_hdmi PRIVATE -O3)
target_link_libraries(zx48_hdmi PUBLIC pico_stdlib pico_multicore hardware_dma hardware_sync
  hardware_clocks hardware_vreg hardware_pio hardware_spi hardware_i2c hardware_pwm hardware_adc)

add_executable(picoZxSpectrum48 "${ROOT}/firmware/zx48_fruitjam.c"
  "${ROOT}/platform/input/jack_input.c" "${ROOT}/platform/input/jack_input_devices.c"
  "${ROOT}/platform/input/jack_tinyusb_input.c" "${ROOT}/platform/text/jack_terminal.c")
target_include_directories(picoZxSpectrum48 PRIVATE "${ROOT}/platform/input" "${ROOT}/platform/text"
  "${FATFS}/fatfs" "${SHARED}")
target_compile_definitions(picoZxSpectrum48 PRIVATE HW_CONFIG=8 PICO_STACK_SIZE=4096
  R8_TITLE="ZX SPECTRUM 48K" R8_DIRECTORY="/roms/ZX48" R8_EXTENSION=".z80"
  R8_BIOS="zx48.rom" R8_PROFILE=1 R8_ASYNC_PAD=0 R8_DEFERRED_PRESENT=1
  R8_SKIP_48K_AY=1 R8_SRAM_HOT=0 R8_ZX_SPLIT_PROFILE=0 R8_BEEPER_STEADY=0
  R8_FUSED_TICK=1 R8_Z80_INLINE=1 R8_HALT_BATCH=1 R8_ZX_PACKED_RENDER=1
  R8_QUEUE_DIAG=1 R8_VARIANT="J_QDIAG" R8_BUILD_VERSION="0.1.8-fruitjam-src"
  JACK_TINYUSB_KEYBOARD_RECOVERY=1 PICO_PIO_USE_GPIO_BASE=1 CFG_TUH_RPI_PIO_USB=1
  PIO_USB_USE_PIO=2 PICO_DEFAULT_PIO_USB_DP_PIN=1 SDCARD_PIN_CS=39
  SDCARD_PIN_SCK=34 SDCARD_PIN_MOSI=35 SDCARD_PIN_MISO=36 SDCARD_SPI=spi0 SDCARD_PIO=pio0)
target_compile_options(picoZxSpectrum48 PRIVATE -O3 -Wall -Wextra -Werror
  -Wno-unused-parameter -Wno-cast-function-type -Wno-missing-field-initializers
  -Wno-attributes -Wno-sign-compare)
target_link_libraries(picoZxSpectrum48 PRIVATE zx48_core zx48_hdmi pico_fatfs pico_stdlib
  pico_multicore hardware_dma hardware_pio hardware_spi hardware_clocks hardware_vreg
  hardware_psram hardware_xip_cache tinyusb_host tinyusb_board tinyusb_pico_pio_usb m)
target_link_options(picoZxSpectrum48 PRIVATE -Wl,--gc-sections -Wl,--print-memory-usage)
pico_set_program_name(picoZxSpectrum48 "picoZxSpectrum48")
pico_set_program_version(picoZxSpectrum48 "0.1.8-fruitjam-src")
pico_enable_stdio_uart(picoZxSpectrum48 1)
pico_enable_stdio_usb(picoZxSpectrum48 0)
include("${ROOT}/cmake/LoaderPartition.cmake")
mac_link_for_loader(picoZxSpectrum48)
pico_add_extra_outputs(picoZxSpectrum48)
