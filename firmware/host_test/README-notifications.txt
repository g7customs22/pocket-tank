Notification and feeding regression

Run firmware/host_test/notification_wiring.py with Python. It checks the production
main-loop integration that cannot run without ESP-IDF, plus feed dispatch wiring.
The unmodified baseline fails the hidden-controls and cropped-timer checks.

From a scratch build directory run firmware/host_test/run-notifications.cmd
(Visual Studio 18 BuildTools, x64). Keep assertions enabled. This compiles the
actual queue, progression, render and native UI code. Persistence and clock are
host stubs; no device or saved data is accessed. The compatibility header only
allows the existing GCC attribute in the scalar model code to compile on MSVC.

Coverage: first-feeding generation in all three habitats; fish milestone, tank
milestone, stage and battery queue/rendering; complete 336x156 popup pixels with
controls hidden and shown; source and destination mapping and output canaries;
six-second expiry; blocked/resumed notices with no repeated cue; dismissal;
silent adoption on slot changes; feeding band boundaries in both zoom modes and
three tilt positions. The integration assertions preserve fish and trim priority.
These checks do not certify physical touch responsiveness or hardware display.
