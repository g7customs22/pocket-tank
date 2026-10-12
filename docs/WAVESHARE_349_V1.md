# Waveshare ESP32-S3-Touch-LCD-3.49 V1 bring-up

Experimental hardware adapter for V1 / Case A. Not V2, not hardware-qualified.
The 14.3M model, tokenizer and save layout remain unchanged. The wider-world
stage adds V1-only world dimensions and separates them from menu geometry.

## Build

Verified with ESP-IDF 5.4.2, target esp32s3. In an exported IDF environment,
start in `firmware` and use separate absolute paths for the build and config:

```powershell
idf.py -B C:/path/to/work/build-v1 `
  -D SDKCONFIG=C:/path/to/work/sdkconfig-v1 `
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.waveshare_349_v1' build
```

Do not reuse the original board's sdkconfig. The V1 option excludes its
SH8601/FT3168 and AXP2101 adapters. AXS15231B is pinned to 1.0.1. This build
uses the same partition table; the model belongs at 0x290000. Building does
not flash the model. Keep its SHA-256:
`50bed3e16a6b11f3496dde21d8784da9067f40ec9f0a77cf97f60a95acb80f6b`.

## Wiring sources

SOURCE_CONFIRMED: [V1 schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49/ESP32-S3-Touch-LCD-3.49-Schematic.pdf)
and [V1 examples at def6edd0b6e1925ed09702eed01a2f181afdf8c1](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49/tree/def6edd0b6e1925ed09702eed01a2f181afdf8c1/Examples/ESP-IDF).

LCD: CS9, CLK10, D0..3=11..14, reset21, PWM8. SPI3, mode3, 40MHz,
RGB565, full frames in sequential 64-row stripes. Peripheral I2C0 SDA47/SCL48;
touch I2C1 SDA17/SCL18, 0x3B at 300kHz. TCA9554 0x20:
EXIO0 touch interrupt (unused), EXIO1 backlight enable, EXIO5 TE (unused),
EXIO6 battery latch, EXIO7 amplifier. PWR16 and BOOT0.
Audio MCLK7/BCLK15/LRCK46/DOUT45. ADC1 channel3/GPIO4, divider factor3.

## Intentional first-stage limits

- The V1 swimming world is now 1344x368, displayed proportionally in 628x172
  pixels with a MENU overlay. Fish sizes, speeds and distance thresholds remain
  in the same world units. This adds space rather than stretching the old frame.
  Open pages use their original 448x172 viewport at 1:1
  pixels, with large UP / DOWN controls to reach all 368 source rows. Scroll
  steps overlap by at least 52 pixels. Page changes return to the top; reset prompts
  start at their title. Setup cannot be bypassed with the shell's TANK button.
  This is a readable scrolling layout, not a reflow of every original page;
  the smallest original labels remain small and rows can cross a scroll edge.
- Battery percentage and charge status are unavailable. Calibrated voltage is
  exposed through the serial battery diagnostics; no fuel gauge is invented.
  Existing percentage-based low-battery alerts are therefore unavailable.
- IMU auto-flip uses measured -Y for buttons-up and +Y for buttons-down.
  Display and touch share the V1 mounting correction. Three consecutive
  opposite-orientation votes are required; flat/sideways poses hold the last
  orientation. The user confirmed rotation and touch work in both orientations.
- PWR short press sleeps; long press requests power-off after release.
  Light-sleep wake uses PWR/BOOT; deep-sleep fallback uses both via EXT1.
  USB power can keep the board alive after battery-latch shutdown.
- Display, raw touch orientation, brightness response, audio quality, retention, sleep
  current, performance and physical usability are NEEDS_VERIFICATION.

## Initial hardware observations (2026-10-02)

- SOURCE_CONFIRMED: model inference and frame transfers run on the connected
  ESP32-S3 with 8 MiB PSRAM. This alone does not confirm a visible picture.
- USER_CONFIRMED: a tone was audible, but the first flashed application showed
  no visible picture.
- USER_CONFIRMED after inverse PWM correction: welcome screen visible, UI
  works, fish swim; text in the original fitted viewport was hard to read.
- SOURCE_CONFIRMED: Waveshare's `lcd_bl_pwm_bsp.h` defines brightness using
  `0xff - level`. GPIO8 feeds the boost feedback network. The adapter now
  uses inverse PWM duty and retains EXIO1 as the separate enable.
- The initial touch response contained `6e` bytes. The vendor example treats
  counts outside 1..4 as released. The adapter follows this handling; actual
  the initial fitted UI subsequently worked in the user's test. The new
  readable shell's touch alignment requires its own physical check.

The readable shell uses one additional 220,160-byte PSRAM framebuffer. Its
pure-C host test checks all menu pixels against inverse touch coordinates,
full vertical coverage, sidebar press/release and drag isolation, setup exit
guard, and framebuffer guards. The wider-world stage changes common geometry
and separates menu width from world width, while preserving the model/tokenizer.

The algae save grid stays at 28x23 cells to retain the frozen save layout.
Wide cells cover more horizontal space; procedural algae blobs keep their size.
The V1 uses namespace `tw49`, importing `tank:save` once without modifying it.
Reset marks import complete so an erased wide tank cannot resurrect the old one.
Directors use the selected namespace too. The wide dirty mask lives in PSRAM
to reserve internal memory for LCD/audio DMA. Hardware startup has about 2.1 MiB
PSRAM free. Measured rendering is roughly 9-12 FPS and inference 11-12 tokens/s.
All six historical save fixtures round-trip in the host test. The actual narrow
save remained byte-identical after wide save/restart. Geometry changes can alter
which neighbors are visible to the model; long-term behavior is not yet assessed.

## Verification and next hardware step

SOURCE_CONFIRMED: original firmware baseline and original-board configuration
after adapter changes compile under IDF 5.4.2. The V1 configuration compiles.
The native C mapping test checks every panel pixel in both orientations,
corners, bounds, margins and display/touch alignment (220,160 pixels).
Run `firmware/host_test/viewport_349_host.c` with common/ and firmware/main/
on the include path, with POCKET_TANK_WIDE_V1 for the wide target. Model/tokenizer
files remain unchanged from upstream b6eb24f64d086887264024dfe7b03275f35bed45.

Before flashing: obtain authorization for reset/bootloader entry, identify the
chip and flash capacity, and preserve two matching full 16MiB flash reads.
If encryption/security settings prevent a restorable backup, stop and report.
Keep backup bytes and their hashes. No erase-all operation is needed.

The first write, if authorized after reviewing the backup, is bootloader at
0x0, partition table at 0x8000, application at 0x10000, unchanged model at
0x290000. Existing factory-firmware data may share these regions; the full
backup is the recovery source. The new partition table changes how firmware
interprets flash. Pocket Tank's inherited NVS rescue logic can initialize
unusable NVS after preserving its raw bytes to the storage rescue region.

After boot, verify model-active logs, RGB colors, all touch corners, setup,
feeding/cards/settings, audio, save/reboot, light/deep sleep and battery latch.
Test battery-only shutdown separately from USB operation. Record memory and
frame/inference timings. Do not call the port accepted until these checks pass.
