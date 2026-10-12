# Three tanks and estimated battery overlay (V1 only)

The TANK button selects one of three independent aquariums. Only the selected tank runs. It keeps the existing sleep/offline progression (including the existing seven-day cap). Switching back to an inactive tank restores its snapshot without applying the interval spent inactive. The slots are Standard, Freshwater and Saltwater; RESET affects only the selected tank. Brightness, sound, and the shared advisor model remain device settings/resources.

The original aquarium migrates to slot 1. The original NVS save and raw NVS rescue are retained. The new journal occupies storage partition offset 0x10000 through 0x29fff (104 KiB), absolute flash 0xaa0000 through 0xab9fff for this partition table. Rescue occupies storage offset 0 through 0x5fff. No filesystem is mounted by this firmware. A missing owner marker permits initialization only if the entire journal arena is erased. Unknown data, incompatible schema, or damaged ownership metadata blocks writes; nothing is automatically formatted.

Each aquarium has two 8 KiB banks, and the active selector has two 4 KiB banks. Records include schema, sequence, size, and CRCs; payload is written before its header and read back. Both aquarium banks are seeded before initial activation. Switching verifies the outgoing save, restores or creates the destination, verifies its snapshot, then commits the selector. Failures preserve the current RAM state; an uncertain selector commit blocks further writes. A reboot can choose whichever valid selector was committed before an interruption. Latest-record corruption falls back to the preceding valid record, so the last save can be lost. Simultaneous loss of both copies is not recoverable automatically.

Schema 0x00010004 snapshots the full tank plus progression context and save timestamp: 4072 bytes on this ESP build. An ABI size assertion guards layout changes, but future field reinterpretations also require an explicit schema change and migration. Transient touch gestures are cleared. The advisor worker is drained before switching/resetting, so a pending answer cannot cross into another tank. Additional persistent working buffers use approximately 16 KiB PSRAM; inactive tanks have no live simulation or advisor task.

The lower-left battery text is transparent except for the small outlined glyphs. A tilde marks its approximate percentage. The existing calibrated GPIO4 ADC measurement remains; readings are filtered once a second with an exponential moving average and three-percentage-point display hysteresis. Generic Li-ion anchors are 3300/3500/3600/3700/3800/3900/4000/4100/4200 mV -> 0/5/10/20/40/60/75/90/100 percent. This is a heuristic, not a cell-characterized fuel gauge. Invalid readings show --%. USB power, load, and battery chemistry affect accuracy; no charging state is inferred.

Before deploying: back up both NVS and the journal arena, check the exact device identity and existing arena contents, and obtain deployment authorization. Flash the application only; never erase flash or overwrite partitions. Rolling back to older firmware reads the retained old NVS save, not newer slot progress. Export/back up the journal before any rollback. This change has host simulation and firmware build validation, not physical touch, battery-accuracy, power-loss, or migration verification on the device.

Hardware references:
- https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49/ESP32-S3-Touch-LCD-3.49-Schematic.pdf
- https://raw.githubusercontent.com/waveshareteam/ESP32-S3-Touch-LCD-3.49/def6edd0b6e1925ed09702eed01a2f181afdf8c1/Examples/ESP-IDF/01_ADC_Test/components/adc_bsp/adc_bsp.c


## Expanded release (2026-10-10, unflashed)

Catalog counts are fish types / shop items (the shop includes residents): Standard 20/23, Freshwater 9/16, Saltwater 10/12. Original species IDs 0..5 and item bits 0..10 remain fixed; additions are appended. The original 1704-byte NVS layout retains only its original five extra-decoration positions. New positions, full simulation state, and breeding timers are kept in the versioned slot snapshots. The journal version changed before any slot build was installed, so there is no deployed older slot schema to migrate. Incompatible journals fail closed. Stored firmware pointers are cleared and reconstructed on load.

Standard retains the current aquarium and its inhabitants, names, balance, decorations, and progression. Freshwater starts with neon tetra and guppy fry, Java fern, driftwood, and river stones. Saltwater starts with clownfish and royal gramma fry, a reef arch, plate coral, and anemone. Both use their own default setup and habitat background/substrate. Existing mixed Standard content is never filtered out of its aquarium. The habitat picker is removed from the tank selector; each new environment stays coherent. Menus paginate filtered choices, and old owned items remain manageable.

Added freshwater fish: neon tetra, guppy, zebra danio, Corydoras, bristlenose pleco. Added marine fish: blue tang, yellow tang, royal gramma, Banggai cardinalfish, firefish, green chromis, pajama cardinalfish, marine neon goby, sixline wrasse. Shapes, fins, markings and cruise speeds differ. Portraits now carry the actual species. Added decorations: Java fern, Anubias, moss mound, driftwood, river stones, slate cave, tiger lotus, reef arch, sea fan, plate coral, anemone, shell bed. Existing background/substrate treatments and growing/trimmable vegetation remain; new decorative plants/corals are procedural ornaments, not independent ecological simulations.

Cleanup roles: pleco and bristlenose graze glass algae; Corydoras prefers fallen pellets through the normal food-seeking behavior; the marine neon goby follows closer and gently reduces a nearby fish's stress as a stylized cleaning visit. Cherry shrimp remain in Standard/Freshwater. Saltwater uses a scarlet/white cleaner-shrimp appearance, with long pale antennae, and the existing bounded food-scrap scavenging simulation. Its shop explicitly labels the implemented in-game role as scavenging. Neither catfish nor marine cleaners remove waste, and no parasite or water-chemistry simulation is claimed. These are virtual habitat catalogs, not real aquarium stocking or breeding advice.

Breeding requires matching species, adult-or-elder stages, hunger <=4, stress <=3, energy >=4, plant nursery cover, and 15 continuous eligible active minutes. It uses the existing courtship and visible fry/naming flow, inherits the parents' species, and allows one offspring at a time up to the existing five-fish firmware limit. A six-active-hour cooldown follows a birth. Fry become juveniles through existing growth (30 minutes of tended age) and adults at three hours. Powered-off/suspended offline catchup advances existing hunger/age but never breeding readiness/cooldown or births. An inactive slot advances nothing. Light-off while the selected tank is still running does not pause its simulation. Breeding is stylized gameplay, not species-specific reproductive biology.

Reset keeps the existing YES/NO confirmation and now saves a separate, redundant pre-reset snapshot before changing the tank. UNDO in the tank selector restores that slot's pre-reset state. It survives autosaves, slot switches, and reboot, remains available after use, and is replaced only by the next confirmed reset of that slot. A failed reset/undo write preserves the current RAM state. Undo snapshots occupy three pairs of 8 KiB banks after the original 56 KiB slot arena; they do not share the rotating autosave banks. Complete the existing new-tank setup flow to return to the selector after resetting.

The selector shows names, fish counts and ACTIVE/PAUSED. Brief food-limit text appears when a feed cannot fit its requested pellets into the eight-pellet pool, including partial capacity. Low battery is debounced over three one-second filtered readings <=10%; a five-second warning appears once, rearmed only after three readings >=15%. The normal approximate percentage remains small and transparent. No charging state is fabricated.

Validation: ESP-IDF build; host tests for 1704-byte legacy save layout, ID stability, habitat filters, default isolation, five-fish Standard migration, inactive timers, active offline catchup, reset/undo across autosaves and reboot, save/undo failures, journal torn-write/corruption behavior, breeding eligibility/cap/cooldown/juvenile growth, feed full/partial capacity, battery warning hysteresis, paginated UI input, touch/hold/grass regressions, and frame/cache/dirty-mask guards for all species/habitats/decorations. Rendered contact sheets and UI were visually inspected. No physical migration, battery-percentage calibration, interaction responsiveness, power-loss endurance or frame-rate validation has been performed for this unflashed release.

The compiled snapshot is 4072 bytes, working snapshot/scratch buffers total 16336 bytes of PSRAM, and the flash arena is 104 KiB. Only one tank and the existing shared advisor run; the fish cap, model weights, framebuffer allocation, and partition table are unchanged.

Classification and role references:
- https://www.aquariumofpacific.org/onlinelearningcenter/species/palette_surgeonfish
- https://www.aquariumofpacific.org/onlinelearningcenter/species/yellow_tang
- https://www.cmaquarium.org/animals/more-marine-animals/firefish/
- https://www.aqueon.com/articles/9-best-aquarium-fish-for-relaxation
- https://www.aqueon.com/resources/care-guides/corydoras-catfish
- https://www.aqueon.com/articles/prevention-and-control-of-nuisance-algae
- https://www.liveaquaria.com/products/neon-goby
- https://www.liveaquaria.com/products/scarlet-skunk-cleaner-shrimp
- https://tropica.com/en/plants/
