# White and UV lighting

BOOT/GPIO0: single short press hides/restores controls; double press selects
White/UV; hold 700 ms toggles light power. Singles wait for a 500 ms double-press
window after release. Minimum press 50 ms, release debounce 30 ms. Double while
off changes the remembered style without turning on. Short-then-long produces
only the power action. PWR remains sleep/power; protected pages and the existing
BOOT-plus-touch reset prompt cancel pending gestures.

Both illuminated styles have identical simulation behavior. All ornamental
materials have UV appearances; fish, snail and shrimp keep their normal colors.
UV-only details include sword veins, fluorescent grass tips, castle mortar and
moss, coral tips, skull eyes, diver markings, chest bands, stone seams, cave lintel,
leaf dots, sea-fan tips, shell ribs and striped neon grass. Static palettes and
bounded opaque detail strokes add no full-screen glow buffer or strobing.
Paired shop previews use a lazy 62,208-byte scratch allocation (firmware malloc
prefers PSRAM above 8 KiB). Neon fern costs 55, neon grass 45, matching existing
fern/grass prices. They are ornamental, with no nursery or physiology effects.

## Save compatibility and rollback

Old item IDs 0..22 and existing positions stay unchanged; new items are 23..24.
The ESP32 tank's original 3976-byte prefix is unchanged. A 16-byte aligned tail
makes tank_t 3992 bytes; progression context stays 88 bytes. Journal snapshots
grow from 4072 to 4088 bytes. Schema changes from 0x00010004 to 0x00010005.
The new reader accepts CRC-valid v4 and v5 records in active, inactive, and undo
banks; v4 tails become zero/White in RAM. Reads alone do not rewrite a record.
Subsequent saves write v5 to the other bank, header last, retaining the previous
valid bank. Later saves may replace both old banks.

Legacy NVS grows from 1704 to 1720 bytes by appending the same tail. Old saves
load through the existing zero-initialized prefix reader with White selected.
No existing NVS offsets, ownership IDs, currencies or prices are changed.

This is forward-compatible migration, NOT safe downgrade compatibility. Old
firmware does not understand v5 journal records and may see stale v4 banks or
block once both banks are v5. Returning to the previous firmware requires its
matching pre-upgrade app/NVS/rescue/journal backup; it loses progress since that
backup. Never downgrade the app alone or restore storage without authorization.
Deployment must verify a fresh live save and CRC/schema/freshness, preserve
rollback backups, write only the app, and compare protected flash before reboot.

## Validation

uv_host.c includes the real renderer/native compositor: 25-item White/UV/Off
render guards, stable cached frames and style invalidation, identical Off output,
identical simulated physiology between illuminated styles, unchanged item IDs,
prices, ownership and placement; paired catalog and three-habitat review images.
button_gestures_host.c tests final arbitration and cancellation.
Workspace qa contains genuine prior-version NVS/journal fixture generators,
v4-to-v5 restore/roundtrip/undo tests and 48 interrupted save/switch cases.
Seven existing regression suites and care/input/usability suites are also run.
Host evidence does not establish device touch feel, frame rate or visual quality.

Unapproved usability proposals (MM:SS, purchase shortfall, numeric care meters)
and duplicate decoration instances are intentionally not implemented.
