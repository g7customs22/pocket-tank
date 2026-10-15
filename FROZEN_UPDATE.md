# Frozen Pocket Tank update (build-only)

Baseline: 7152d66 preserves the completed floor/zoom work (be4374b) and the installed b163306 notification/feeding patch. The installed binary is recorded separately; source commit IDs and embedded firmware version strings are not interchangeable.

Changes:
- Brightness choices 5/10/30/60/100, persisted in the existing brightness key. The low levels combine the existing visible backlight floor with pixel gain; they are not measured luminance or power percentages.
- Drag scrolling in menu/shop and their detail pages. Existing arrows remain. A drag captures its release; settings number gestures and aquarium cleaning retain their own handlers.
- First touch after automatic lights-out or a grace-sleep wake is consumed through release.
- Redraw ceiling of 30 fps during interaction, 8 fps idle and 5 fps lights-off. Actual panel throughput may be lower. Touch polling and fixed 30 Hz simulation steps are outside the redraw gate. Existing maximum elapsed-step policy remains. Lights-off suppresses decorative bubbles and freezes decor/plant sway; fish and game state still advance. Fractional snail grazing removes per-tick rounding dependence.
- Six recent events, newest first, labeled by habitat, kept for this session. Open MENU > HISTORY. Boot/switch adoption does not replay historical milestones. No history bytes added to saves.
- Care progress shows m:ss; help explains that ordinary care lapses pause, severe neglect decays, and changing the bound pair resets care. Existing real blocker descriptions and independent nursery-cover status preserved.
- Saltwater Turbo Snail uses the existing glass-cleaning mechanics with a marine palette and explicit shop label. Pleco/bristlenose selection restricted to Freshwater; existing saved residents are retained.
- Append-only Lawnmower Blenny and peaceful Octopus species. Blenny grazes algae; Octopus makes bounded shelter/exploration movements and uses procedural arms. Existing schooling/cleaner roles were already present, so no duplicate roster expansion.
- Copy sales require confirmation. Purchase/sale feedback and truthful manual serial-save confirmation; autosaves do not produce feedback. Insufficient-credit detail states the missing amount. Existing reset/undo and firmware version display preserved.

Validation artifacts in ../frozen-qa and existing floor-qa/prototype-qa:
- Host controls/cadence/history/species/UV/off buffer checks, all-habitat snail cleaning without player-cleaning credit.
- Production notification queue, hidden/shown-controls popup pixels, feed boundaries and zoom.
- Native floor/zoom reference-pixel equality, touch mappings, production fish picking, foreground cleaning and framebuffer guards.
- Growth/nursery/care and inactive-slot/offline policy.
- Actual 32-bit device layout remains tank 4072 bytes; v4/v5 migration, reset/undo, 96 complete/partial interrupted writes; new species survive save/reboot without balance changes.
- Five brightness levels save/reload through mocked NVS; invalid levels rejected.
- Native previews visually inspected for settings, history, and species in White/UV/Off.

Not yet verified on hardware: low-level brightness legibility, touch feel/wake contact, render throughput, power savings, physical animal appearance and new feature operation. No flash or device reset belongs to this checkpoint. Before deployment, take a fresh matched app/NVS/all-slot backup and preserve actual current progress. The historical +500 credit grants are already completed and must not be replayed.
