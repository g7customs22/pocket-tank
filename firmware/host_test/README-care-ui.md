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
- An existing REST goal restored with energy 3 continues recovery. No new fields
  are added to fish, tank, progression or the slot-journal payload/schema.
- Live-like Willow hunger/energy blockers; exact breeding thresholds; age,
  species, habitat, plant-cover and cooldown feedback.
- Open-panel 59-to-60-second update and failure reset preserve HOW and navigation.
- All 20 species and four stages match the aquarium core at the same portrait
  pose; locked portraits are a single ink; classic palette previews are preserved;
  the actual milestone row uses the species-aware portrait.

The test writes `care-blockers.ppm` for visual inspection. Existing habitat,
species bounds, catalog/breeding, slot persistence, pixel mapping and 328
interrupted-write regression cases are also run against the candidate.

Behavior tradeoff: REST owns the goal during exhaustion recovery, including
overriding learned decisions and finishing naturally selected low-energy rest.
This does not change the breeding rule: 15 continuous qualifying active minutes
are still required, and hunger/energy/stress failures still reset that timer.
Autonomous survival and recovery do not promise uninterrupted breeding care.
