# RTC offline-care guard

Battery removal is an unknown-time condition. Never write firmware build time into the RTC. The platform clock is trusted only after valid PCF85063 calendar/oscillator status and two advancing reads 1.1 seconds apart; saves stamp zero when untrusted. Both journal and legacy NVS loaders share the interval policy. Backward, unknown and greater-than-seven-day intervals skip catch-up; a valid normal interval up to seven days still advances the selected tank only. Save layouts remain v5/1720-byte NVS.

A valid-looking clock previously seeded by old firmware cannot be distinguished from correct time without an external reference. No clock correction or state restoration is part of this patch. Genuine absences exceeding seven days now resume without offline simulation. Runtime sleep measured independently remains unchanged.

Validation: clock_policy_test.c covers OS flag, BCD/calendar/leap-day validity, stopped/backward/jumping RTC and advancing reads. rtc_slot_test.c runs production journal and legacy NVS loaders against unset, backward/future-saved, two-hour, exact-cap, excessive-gap and repeated unknown-to-valid boots. The existing migration, inactive-tank freeze, rollback, undo and reward-persistence cases also pass. ESP-IDF 5.4.2 build passes unchanged schema assertions.
