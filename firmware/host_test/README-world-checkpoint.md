# Paused experimental checkpoint

Paused for the deployed-release double-press hotfix. No flash or push.
Branch prototype/tilt-world starts at deployed74d3237. Hardware remains on74d3237.

Audit: QMI8658 at0x6b, acceleration31.25Hz configured, gyro off, existing host poll
250ms. Actual prior boot samples429/-16189/637 are healthy; no new sensor access.
World1344x368 already persists; prototype camera presents640x172 instead of
compressing the entire world. Proposed layers0.40/1/1.25, 2deg dead zone,
220ms smoothing, +/-25deg travel, gesture freeze, recenter and stale/railed guard.

Experimental host-only modules: bounded eight-copy objects/four bubble sources,
per-fish stable target/retarget, advisor/arrival/milestone hooks, separate glass
coordinate transform, viewport vector renderer and scripted tilt tests.
Extra persistent payload is64 bytes, kept outside unchanged tank_t. Proposed v6
snapshot appends to v5 prefix4088 =>4152 bytes, below8160-byte journal limit;
NVS would append64 to1720 =>1784. Actual migration is NOT implemented. No live
save or layout was changed. Camera/selected-source state is transient.

Work is NOT VALIDATED: initial host link lacked model/tokenizer dependencies;
the follow-up build had a q4.c filename typo (actual file q4_model.c). The workspace
run script is corrected but has not been rerun after the priority pause. No
preview frames, performance claims, save-compatibility claims or firmware build
should be inferred. Next step: complete host build/tests, inspect native frames,
test wire migration and interruption handling, then review hardware integration.
