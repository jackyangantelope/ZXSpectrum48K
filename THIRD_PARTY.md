# Third-party sources

| Component | Source revision | License | Local changes |
|---|---|---|---|
| chips | [floooh/chips](https://github.com/floooh/chips) `ca7d7ddd3ba77b48685d24120cf413ea53786767` | zlib/libpng (`deps/chips/LICENSE`) | `systems/zx.h` externalizes backing storage, adds optional render/timing hooks and disables whole-struct snapshots for the pointer layout; `chips/z80.h` adds optional SRAM placement and tick inlining; `chips/beeper.h` adds a filter cache. Original emulation algorithms and notices are retained. |
| pico_shared | [fhoedemakers/pico_shared](https://github.com/fhoedemakers/pico_shared) `df683bf00ff95e1ceda88f132bbf72f8cc7cadb0` | GPL-3.0-only (`deps/pico_shared/LICENSE`); FatFs retains its own license (`deps/pico_shared/drivers/pico_fatfs/LICENSE`) | `drivers/pico_hdmi/hstx.c`, `hstx.h`, and `video_output.c` add a nonblocking double framebuffer handoff. Other included files are from the public snapshot. |

The port-specific `firmware/zx48_fruitjam.c` and `platform/retro8/*` adapt the chips machine to a bounded external snapshot loader, frame pacing, PCM queue, HDMI presentation, and Fruit Jam SD/USB services. `platform/input/*` and `platform/text/*` are shared port code. SDK, Pico-PIO-USB, and TinyUSB are external build dependencies. No third-party ROM or game data is distributed.
