# Care feedback and species portrait regression

`care_ui_host.c` compiles the production renderer directly into the test so
modal text, HOW state and navigation can be inspected without adding a firmware
testing API. Persistence and wall clock are stubbed; no device is accessed.

Run from the repository root with a C11 compiler (assertions enabled):

```sh
cc -std=c11 -O2 -DPOCKET_TANK_WIDE_V1=1 -DPOP_CAP=5 -Icommon \
  firmware/host_test/care_ui_host.c common/tank.c common/advisor.c \
  common/icons.c common/progression.c common/setup.c common/notice.c \
  common/battery.c -lm -o care_ui_host
./care_ui_host
```

Windows/MSVC: use the same sources with `cl /std:c11 /O2 /Gy`, `/D` and `/I`
in place of `-D` and `-I`, `/D_CRT_SECURE_NO_WARNINGS`, and `/link /OPT:REF`.
Do not separately compile `common/render.c`: the host test includes it.
The MSVC route was executed; the portable command is provided, not verified.

Coverage:

- Exhaustion at energy <=1.5 enters REST. Repeated late bubble responses cannot
  interrupt REST below 6. Recovery releases at 6 without immediate oscillation.
- Fish recover independently, including the second fish on a requesting frame;
  the advisor traversal origin is fixed while the next-frame index changes.
- An existing REST goal restored with energy 3 continues recovery. No fish/tank layout or journal schema changes. Care tracking uses formerly
  zeroed progression-context padding with frozen-offset assertions.
- Live-like Willow hunger/energy blockers; exact breeding thresholds; age,
  species, habitat, plant-cover and cooldown feedback.
- Open-panel 59-to-60-second update and care pause preserve HOW and navigation.
- All 20 species and four stages match the aquarium core at the same portrait
  pose; locked portraits are a single ink; classic palette previews are preserved;
  the actual milestone row uses the species-aware portrait.

The test writes `care-blockers.ppm` for visual inspection. Existing habitat,
species bounds, catalog/breeding, slot persistence, pixel mapping and 328
interrupted-write regression cases are also run against the candidate.

Behavior tradeoff: REST owns the goal during exhaustion recovery, including
overriding learned decisions and finishing naturally selected low-energy rest.
The local gameplay addition accumulates 900 qualifying active seconds for a bound
pair. Ordinary health/cover lapses pause; hunger >=8.5 or stress >=7.5 for 300
continuous active seconds starts decay at 0.2 earned seconds per active second.
Recovery clears the severe-care clock. Pair/species/habitat invalidation resets
progress; birth resets progress and starts the existing 21600-second cooldown.
Other slots/offline do not advance these clocks. Existing 88-byte progression
contexts have zeroed padding; six padding bytes store binding and centisecond
grace state (at most 0.005 second quantization on capture). All preexisting offsets,
offsets remain unchanged for the care-only checkpoint. The subsequent UV feature
appends a tail and migrates the save schema; see README-uv.md for exact compatibility.

Future gameplay rewards are doubled: meals 4; stages 10/20/50; birth 40; trust 30;
algae/trim chores 50. Prices, sale refunds, direct grants and stored balances are
unchanged. Legacy initial ledger back-pay remains at base rates. Credits and toast
aggregation saturate safely at INT32_MAX; capped awards show actual credited coins.

Additional tests cover care accumulation/pause/decay/recovery, old padding, context
switch/reload, pair changes, birth/cooldown, natural rules over one active hour,
reward paths/labels/repeated ticks/overflow, and unchanged manual/refund credits.
`usability_host.c` exercises the production native 640x172 compositor using isolated
states, writes screenshots, and checks navigation and species/tank picker targets.

Physical BOOT/GPIO0 light key (separate from the power key): short press hides or
restores the current page/card, retaining renderer modal and native scroll/picker
state. From plain aquarium with no remembered page it opens MENU. A captured touch,
setup or reset confirmation prevents toggling. New navigation/dismissal discards
remembered page state; tank switching cannot restore a previous tank's card.
Long hold at 700ms toggles aquarium lighting once. Short release acts after only
30ms switch debounce and a 350ms double-press window. Double press changes
White/UV style, including while off; long hold retains that style. Clean view
starts with all controls hidden; the entire 640x172 area shows aquarium pixels and
routes touches to aquarium interactions, including areas formerly under buttons.
Fish taps and decoration holds do not reopen panels in clean view. A short press
restores controls and any remembered page/card, or opens MENU if none was stored. Existing BOOT-plus-touch reset prompt
is preserved and consumes the gesture. Boot-held keys are ignored until released.
`button_gestures_host.c` tests the production classifier. The workspace QA fixture
extracts the production visibility function and verifies hide/restore/protection.
