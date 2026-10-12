Final approved mapping: single toggles aquarium light power after the 500 ms double window; long (700 ms) shows/hides controls and selectors; double toggles White/UV without changing power. Short-then-long triggers controls only. From clean aquarium, long reveals the direct selectors. Remembered pages restore normally.

# BOOT double-press hotfix (not flashed)

Base: deployed `74d3237`. Branch: `fix/forgiving-double-press`.
Scope: BOOT GPIO capture, gesture arbitration, tests and button documentation only.
No save schema, tank simulation, UV rendering or physical hardware changes.

## Diagnosis

- USER_CONFIRMED: "too fast, menu, too slow, menu, just right, uv".
- SOURCE_CONFIRMED: GPIO 0 is configured as a pulled-up input. No GPIO interrupt or separate debounce task fed the deployed recognizer. `sleep_button_poll` read it once per tank/render iteration.
- SOURCE_CONFIRMED: the final deployed boot log reports 6.5 fps, render 52.0 ms and flush 63.6 ms. The measured average frame interval is about 154 ms; individual intervals were not instrumented.
- SOURCE_CONFIRMED: deployed release debounce needed an up observation followed by another up observation at least 30 ms later. A down observation before that second up cancelled the release even when the physical gap had lasted longer than 30 ms. A complete press between polls could disappear entirely.
- SOURCE_CONFIRMED: the old 350 ms upper window starts at the *observed first release*, not second release. A captured second press already could finish after that window. At the deadline the old code dispatched the single before processing an incoming down edge, then returned without recording that edge.
- HOST_REPRODUCED: frozen deployed recognizer with 150 ms GPIO polls recreates both fast-tap loss and a narrow timing region. With 200 ms presses,40/100 ms gaps never produce UV across 150 phase offsets;300 ms gaps work in all 150;350 ms gaps work in 100;500 ms gaps work in 0.
- GPT_INFERENCE: this explains the user's timing-dependent menu symptom. The specific physical attempts were not electrically captured, so their precise timings are unknown.

## Change

A 5 ms ESP timer callback captures timestamped GPIO transitions into a 64-edge buffer. It never reads/mutates tank, touch or UI state. A short critical section protects capture/snapshot. The tank task replays transitions chronologically and dispatches actions on its existing thread. Stable releases are resolved before a following down edge even when no render iteration occurred during the gap.

The first-release-to-second-press window is 500 ms, inclusive at a captured down edge. A second short press can complete naturally beyond 500 ms. Long 700 ms suppresses single/style, including short-then-long. A late unrelated press is recorded even when an overdue single is dispatched. Minimum press 50 ms and stable release 30 ms remain. Triples form a double plus a subsequent single. Boot-held buttons are ignored until release. Confirm/setup suppress gestures. Capture is cleared/paused over sleep and restarted on resume; overflow discards the batch and disarms rather than inventing an action. Touch chord checks exclude touches newer than replayed edge timestamps.

The buffer costs roughly 1 KB static and 1 KB stack; no allocations per sample. ESP timer task priority is supplied by IDF, independent of the rendering task. Nominal 5 ms sampling is not a hardware edge guarantee during interrupt/cache/scheduler stalls; device responsiveness remains to be verified.

## Verification

MSVC C11 `/W4 /WX`:
- 18 focused assertions/cases: exact 50 ms,30 ms,500 ms,700 ms boundaries; second release after deadline;699 ms second hold; prolonged hold one-shot; short-then-long; bounce; short rejection; triples; boot-held; reset chord; confirm/setup cancellation; buffer overflow.
- 187,200 trials: widths 50..300 ms in 10 ms steps; gaps 30..500 ms in 10 ms steps;150 phase offsets with 150 ms tank dispatch. Old 45,850 successful; new 187,200 successful. Success means exactly one UV and zero menu/power actions.
- 20,000 trials with 400 ms delayed tank dispatch: all pass.
- 57,750 additional non-grid trials: widths 51..289 ms in 17 ms steps; gaps 31..487 ms in 19 ms steps;154 phases and 154 ms dispatch. All pass.

These tests execute the production capture helper and production recognizer, with a simulated 5 ms sampler and tank-thread dispatch. They do not emulate FreeRTOS scheduling or electrical switch behavior. Existing 50 ms/30 ms limits intentionally exclude extremely brief pulses/gaps. See `button_deployed_74d3237.h` for the frozen baseline; only symbols were renamed. The full acceptance matrix and test log are in the workspace `button-hotfix-qa` directory.

Run MSVC host test with the common and firmware/main include directories. It accepts an optional CSV output path as argv1; default `button-acceptance.csv`.

ESP-IDF 5.4.2 full firmware build passed (app 1,281,504 bytes, 51% partition free). Physical acceptance remains pending explicit flash approval. No flash is authorized for this hotfix. The larger-world prototype is paused at 0bb2d2e; its validation remains incomplete.
