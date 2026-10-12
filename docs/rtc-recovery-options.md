# RTC incident: evidence and recovery choices (not applied)

## Confirmed evidence
- USER_CONFIRMED: battery was removed.
- SOURCE_CONFIRMED: recovered running f5ec9a3 logged an unset RTC and seeded Oct 12 2026 19:08:49. Its saved timestamp was 1790966021. The old loader clamped the resulting gap to 604800 seconds.
- SOURCE_CONFIRMED: pre-boot storage matched the fresh backup byte for byte. The jump happened in runtime offline simulation, not corrupted flashing.
- SOURCE_CONFIRMED: Freshwater's age increased from 38.88677h to about 80.9h: exactly 42h from 168h at the 0.25 sleep-growth factor, plus live time. Snapshot sequence 469 backs up age 80.99798h, tank clock 747742.25, care 105.501854s, balance 319/earned 519. Original sequence454 had clock142449.515625, same care, balance157/earned357.
- SOURCE_CONFIRMED: current journal CRCs pass. Rescue region remains byte-identical. Standard and Saltwater also have later snapshots, with small live-time increases; they are not byte-identical to pre-update copies. Do not replace all slots.
- GPT_INFERENCE: battery removal plausibly caused the RTC oscillator-stop/unset state; the electrical cause was not measured.

## Fields affected
Direct sleep simulation: tank clock; fish hunger/energy/stress with irreversible clamps, speed/target_speed/bored/goal_age/ask_age; idle; pellet age/alive; vegetation/fronds, coral, cluster; snail/shrimp sleep effects; algae and its accumulator/RNG; progression ages, stage/growth; full-night milestone. Live updates then changed positions, goals and physiology further. The full-night bit changed tank_ms_bits1985 to2017 (bit32).

Later actions/progression must be retained where possible: player_feedings58 to61, trims49 to54, cells_cleaned282 to375, snail_grazed184 to217, credits157 to319 and earned357 to519. Ownership52739 and plant_x1265 remain. UV style changed from1 to0. These deltas are confirmed; the origin of every reward cannot be proven from counters alone. Additional user-authorized +500 grants are recorded separately and must survive any subsequent recovery.

## Options requiring a decision before writes
1. Keep current state and install only prevention. No lost later actions; artificial age/growth and full-night milestone remain.
2. Targeted compensation: subtract exactly604800 from Freshwater clock and151200 seconds from both progression ages, recalculate normal age-derived growth/stage; clear only the newly introduced full-night milestone (and corresponding seen flag if set). Keep credits including grants, purchases, names, placements, UV/manual-light choices and live activity counters. This reverses additive clock/age changes only. Hunger/stress, foliage, algae and RNG cannot be exactly unwound after clamping/live interactions. Do not describe this as full restoration.
3. Restore only Freshwater's complete verified pre-update snapshot (schema v5 sequence454), rebased to unknown timestamp0 under the guard. This restores the exact old fish/garden/care state but discards all subsequent Freshwater feeding, trimming, cleaning, rewards and settings. Explicitly replay the separately requested +500 credit grant once; optionally retain current balance/settings only if separately agreed. Standard and Saltwater stay current. No all-slot or firmware rollback is necessary.

No option has been applied. Preventive source commit8e574f1 and fish-selection commit aa6396a are compiled but not flashed. Guard does not repair an already-seeded plausible RTC: no external current-time correction has been authorized/applied. Zero timestamps safely establish a fresh baseline later. Genuine absences over7days intentionally skip catch-up. Small plausible forward errors remain indistinguishable from genuine downtime without external time truth.

## Fish selection
USER_CONFIRMED: fish cannot be selected from the closed-menu tank. SOURCE_CONFIRMED: the clean-view branch sends every tap directly to tank_touch_tap and exits before creature hit testing. Commit aa6396a routes it to existing fish/snail/shrimp hit testing, bypassing invisible overlays. Empty-water taps retain their action. Native UI/mapping host regression and IDF build pass; physical on-device tap validation is pending deployment.

## Authorized credit grant completed
Direct user request: give Saltwater and Freshwater500credits each. Fresh backup retained in outputs/rtc-diagnosis/pre-credit-save.bin. Only sd_balance and sd_earned increased by500 in each latest snapshot, using inactive journal banks with new sequences and valid CRCs; previous banks retained. Freshwater319->819 (earned519->1019), Saltwater8->508 (earned8->508). Full172032-byte save-region readback equals the exact planned result; Standard, rescue, metadata, all undo records, and other payload fields unchanged by this write. credit-verification.json records addresses and hashes. Existing f5ec9a3 restarted normally, active Freshwater; both serial state reads report819. Saltwater508 is independently verified in its persisted record. No new application was flashed. Live RTC now runs from the old seeded value; its wall time is not independently corrected.

Additional fish verification: generated host harness compiles the production creature-hit block and passes moving-fish press/current-position targeting, selection switching/toggle, empty-water dismissal/tap, and hidden-overlay bypass. Full physical touch integration remains unverified until deployment.
