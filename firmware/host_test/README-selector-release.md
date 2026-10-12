Final approved mapping: single toggles aquarium light power after the 500 ms double window; long (700 ms) shows/hides controls and selectors; double toggles White/UV without changing power. Short-then-long triggers controls only. From clean aquarium, long reveals the direct selectors. Remembered pages restore normally.

# Direct selector + BOOT timing release

Approved by user `proceed` after the three-button preview; app-only deployment authorized. Based on deployed74d3237 and button hotfixc38c0c1. No save layout/version change.

Controls visible: STANDARD/FRESHWATER/SALTWATER buttons168x48, active gold outline, separate MENU80x48. Controls hidden: no selector visuals or hitboxes. The visible selector band consumes its gaps to prevent accidental aquarium actions. Same-target release and16px drag cancellation remain. Active tank produces no switch request. MENU > TANKS retains management/UNDO access.

Switching retains existing advisor quiesce, save-before-switch, checked journal activation and rollback behavior. No simulated care, balance, inventory or fish identity changes.

Validation: native UI bounds/drag/clean-view/guard tests, active-slot no-op without writes, existing switching/restore/rollback suite,328 interrupted ownership/migration/switch/reset/undo cases,264950 BOOT timing simulations and18 focused cases. Device save schema remainsv5, snapshot4088bytes; firmware compiler static assertions enforce offsets.

Deployment procedure: capture current live state/save, fresh16MiB backup, validate journal CRC and save freshness; app-only write; independent full readback and protected-storage comparison before boot; verify build and live state after boot. Never restore an older save by default.
