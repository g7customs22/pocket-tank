# Habitat nursery regression

This test compiles the production tank, progression and slot journal code.
Only the ESP flash/allocation/log ports and wall clock are mocked. It never
reads or writes a device or an existing save.

From the repository root, on a C11 host with a C compiler:

```sh
cc -std=c11 -O2 -DPOCKET_TANK_WIDE_V1=1 -DPOP_CAP=5 \
  -Ifirmware/host_test/slot_mocks -Icommon -Ifirmware/components/tankcore \
  firmware/host_test/habitat_growth_host.c \
  firmware/components/tankcore/slot_system.c \
  firmware/components/tankcore/slot_journal.c common/tank.c common/progression.c \
  -lm -o habitat_growth_host
./habitat_growth_host
```

On Windows, from an x64 Visual Studio Developer command prompt:

```bat
cl /nologo /std:c11 /O2 /Gy /DPOCKET_TANK_WIDE_V1=1 /DPOP_CAP=5 /D_CRT_SECURE_NO_WARNINGS /Ifirmware/host_test/slot_mocks /Icommon /Ifirmware/components/tankcore firmware/host_test/habitat_growth_host.c firmware/components/tankcore/slot_system.c firmware/components/tankcore/slot_journal.c common/tank.c common/progression.c /Fehabitat_growth_host.exe /link /OPT:REF
habitat_growth_host.exe
```

Keep assertions enabled (do not define NDEBUG). `--trace-only` runs the
state/growth checks without the corrected-checklist assertions, for comparing
the unchanged baseline. The Windows/MSVC route was executed for this change;
the portable command is provided but was not run here.

Coverage: all three habitats, nursery cover at initialization and after
trimming, five active hours of regrowth, exact per-slot plant preservation
across a day inactive, selected-slot offline growth after reboot, mixed-species
starters, the 15-minute care boundary and staged birth, trimming after care,
and a bought sword plant as the only eligible nursery.

The baseline incorrectly reports the grass requirement as unmet when a
nursery exists but breeding care time is zero. The fix reports actual cover
and puts the care timer on PAIR + CARE. Growth rates, eligibility, paused-slot
semantics, storage schema and firmware behavior are unchanged.
