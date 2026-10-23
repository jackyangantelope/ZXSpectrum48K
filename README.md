# ZX Spectrum 48K for Adafruit Fruit Jam

This is a source release of a ZX Spectrum 48K emulator for the RP2350-based Adafruit Fruit Jam. It loads user-provided `.z80` snapshots and a user-provided `zx48.rom` from `/roms/ZX48` on the SD card. No ROM, snapshot, game, or prebuilt firmware is included.

The machine core is adapted from [floooh/chips](https://github.com/floooh/chips) revision `ca7d7ddd3ba77b48685d24120cf413ea53786767`. The port supplies PSRAM-backed screen and memory storage, a bounded snapshot loader, USB keyboard input, HDMI video, and original beeper output. `THIRD_PARTY.md` records the included dependencies and changed source files.

## Build

Install the Raspberry Pi Pico SDK (RP2350 support), Pico-PIO-USB, CMake, Ninja, and an `arm-none-eabi` toolchain. On Windows, run `scripts/build-release.ps1` with the paths to these tools:

```powershell
.\scripts\build-release.ps1 -PicoSdkPath C:\path\to\pico-sdk -PicoPioUsbPath C:\path\to\Pico-PIO-USB -ToolchainPath C:\path\to\arm-toolchain -NinjaPath C:\path\to\ninja.exe
```

The output is `build/fruit-jam-release/picoZxSpectrum48.uf2`. This image uses the loader application partition at `0x10080000`; use it only with a compatible loader. Place your own legally obtained ZX Spectrum 48K ROM at `/roms/ZX48/zx48.rom` and `.z80` snapshots in `/roms/ZX48`.

## Controls and status

USB keyboard: Up/Down select a snapshot; Enter loads it; Escape returns to the menu. During a game, keyboard keys are passed to the emulated machine.

The source is derived from the `0.1.8 J_QDIAG` candidate. The original hardware run showed approximately 50 simulated frames per second in its steady interval, but had audio queue underruns/overflows and did not pass full stability acceptance. The Fruit Jam retarget has been compile-checked only; no Fruit Jam gameplay, audio, input, or long-duration hardware test is claimed. The menu and firmware remain experimental.

The root `LICENSE` is GPL-3.0-only for the combined release. Individual third-party files retain their own notices and licenses. The port's own files use BSD-3-Clause notices where marked.
