# UTC clock recovery

Settings > CLOCK opens a native 640x172 editor. Tap year/month/day/hour/min/sec,
then minus or plus; SAVE validates the complete date, CANCEL discards edits.
All fields are UTC (2024–2099), with no fixed local/DST offset. Swipes do not
activate controls. An unset clock starts with clearly labeled placeholder fields.

Console `clock` reads status, control/calendar bytes, oscillator-stop flag,
STOP flag, validation and system epoch. `clock set <UTC Unix seconds>` uses the
same transaction as the UI. It runs on the tank task, without concurrent gameplay.

Before hardware writes, normal slot saving commits timestamp zero with the live
gameplay state. RTC calendar writes use the existing PCF85063 seconds-through-year
register mapping; explicit setting clears only the 12/24-hour control bit to
select 24-hour format. Other control bits are preserved; STOP is rejected.
Readback must match the requested epoch and advance normally. Only then are the
system clock and a new normal-save baseline established. No offline progression
function is called. Inactive slots remain frozen and get fresh timestamps when
activated. The elapsed awake seconds spent doing the operation remain ordinary
simulation time, independent of the size/direction of the clock correction.

Before final commit, hardware failure attempts a checked register rollback and
leaves offline care disabled. If final baseline commit fails, the new RTC is kept:
either the zero baseline or the new timestamp is safe; restoring an older clock
after an ambiguous save would not be. This case reports failure and requires
inspection, not automatic retries. A rollback failure is reported explicitly.
Power interruption after the zero baseline cannot cause retrospective catch-up.

Host tests: run-rtc-set.cmd exercises the production driver with mocked I2C,
clock and storage, including calendar validation, both correction directions,
restart, stopped clock, partial hardware-write rollback and save failures.
run-clock-ui.cmd exercises native hit targets, swipe cancellation, empty-contact
capture and renders the editor. Existing timing/gameplay regressions are retained.
