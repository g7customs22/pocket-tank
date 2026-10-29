# Clock recovery after battery depletion

The V1 schematic supplies the PCF85063A RTC from 3.3 V and VBAT through diodes. There is no independent RTC battery in that drawing. Losing both supplies can erase the calendar and set oscillator-stop; the RTC must then receive a known time before offline progress is safe.

SOURCE_CONFIRMED: On 2026-10-29 the connected device reported an invalid calendar and OS=1 before any reset. Its installed application matched the October 20 persistent-sleep build. Restoring current UTC through the existing transactional setter cleared OS; a short timed deep-sleep/wake retained valid, advancing time. Standard's actual saved goal was absent; Freshwater and Saltwater already had it.

USER_RECALL: Complete battery depletion since the prior correction was possible.
GPT_INFERENCE: Depletion is the leading explanation for the calendar reset. The precise historical event and long-duration retention are not proven by the short test.

## Recovery behavior

With `CONFIG_POCKET_TANK_WIFI_TIME_RECOVERY=y`, invalid RTC time triggers one Wi-Fi/SNTP attempt after the aquarium has loaded. A valid clock causes no Wi-Fi connection. The attempt stops after success, disconnect, startup failure or 45 seconds, and is cancelled before sleep/power-off. There is no retry loop. A later boot or `clock retry` can try again.

The SNTP hook only hands a sample to the tank task. It does not call `settimeofday`. Reply source and echoed request timestamp are checked by lwIP (`SNTP_CHECK_RESPONSE=2`); sample dates must fit the RTC calendar and cannot be stale. SNTP is not authenticated time. The server is `time.cloudflare.com`.

Only the existing `rtc_port_set_utc` transaction sets the RTC/system clock. It saves an unknown timestamp first, verifies the RTC is advancing, and saves a fresh baseline. Unknown time offline receives no growth, money, or achievement credit. Inactive aquariums retain their data and refresh their baseline on selection. A subsequent uninterrupted six-hour sleep with valid time can earn the goal.

No snapshot schema, achievement threshold, feeding reward or aquarium pause rule changes.

The V1 recovery configuration limits Wi-Fi to four static RX/TX buffers and eight dynamic RX buffers, enables external-RAM allocations where supported, and disables optional Wi-Fi IRAM optimizations. The installed build had only about 51 KB internal heap free during operation; the new runtime margin must be measured on hardware.

## Credential setup (after an approved flash)

Run `python firmware/host_test/configure-time-wifi.py` on the connected PC. It checks the expected device identity and privately prompts for the SSID and password. It never writes credentials to a file or passes them in process arguments. It requires Python with pyserial. Use a 2.4 GHz WPA2/WPA3 network with a password of 8–63 UTF-8 bytes and SSID of 1–32 bytes; enterprise/captive-portal networks are not supported.

Credentials are stored in a separate `time_wifi` NVS namespace on the device, not in source or aquarium records. The existing firmware configuration does not encrypt NVS. Wi-Fi uses RAM configuration so the driver does not create another persistent credential copy. The console does not echo the setup command; its buffer is cleared after processing and overlong commands are discarded.

`clock network` reports configured/valid/active flags without revealing network details. `clock retry` requests one new attempt only if time is invalid. `clock sync` explicitly requests one bounded sync even with valid time, for USB commissioning. It uses the same safe clock transaction. Memory free/largest/minimum readings are logged around the attempt. `clock` reports RTC validity. Existing manual clock setting remains available if the network is unavailable.

## Verification

Run `run-time-recovery.cmd` from a scratch directory on Windows with Visual Studio Build Tools. It exercises the production recovery controller using mocked network, NVS, RTC and clock ports: callback isolation, normal completion, timeout/no retry, disconnect, sleep cancellation, manual-clock override, stale/out-of-range samples, RTC failure, all startup failures, malformed/maximum credentials and failed credential commit.

Also run `run-rtc-set.cmd`, the x86 actual-save replay and all-slot threshold tests. Compile a separate V1 firmware build with Wi-Fi and recovery enabled; confirm the ELF contains the strong `sntp_sync_time` override and lwIP is built with response checking.

Hardware acceptance still requires the new firmware to be installed with explicit approval, credential setup, a network recovery after an invalid clock, failed-network behavior, radio shutdown and an actual six-hour sleep. A host test or successful build does not establish those results. Do not intentionally drain the battery to test recovery; use controlled invalidation only after backup and separate hardware-test authorization.

Sources:
- https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.49/ESP32-S3-Touch-LCD-3.49-Schematic.pdf
- https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf (oscillator-stop behavior)
- ESP-IDF 5.4.2 `components/lwip/include/apps/esp_sntp.h` documents overriding `sntp_sync_time`.

## Authorized hardware commissioning, 2026-10-29

SOURCE_CONFIRMED: App-only installation and exact readback passed on COM23 / 10:20:BA:3D:2F:FC. Before restart, all aquarium journal/rescue/meta/undo bytes, NVS, partition table and bootloader matched the fresh backup. The model partition was outside the write range. Credentials were provisioned privately over USB and survived the second app-only installation and timed sleep.

The first live Wi-Fi attempt failed to create its task with only 18,923 internal bytes free. The CPU-only 63,984-byte card cache now uses existing PSRAM when recovery is enabled, reserving internal memory for networking. The corrected build connected, received a time sample, committed the RTC through the safe setter and stopped Wi-Fi. Measurements: 84,479 bytes before; 47,931 at sample, largest block 38,912; minimum reported 45,571; 75,051 after radio shutdown. External RAM remained about 2.1 MB free. Display logs settled at 4.6-4.8 fps; this does not establish physical UI acceptance or card-open performance.

The hardware test used explicit `clock sync` with already-valid time. Automatic invalid-clock recovery and network-failure paths have host coverage; actual total-power-loss boot recovery and six-hour physical sleep remain NEEDS_VERIFICATION. The final build retained valid time/configuration through a 15-second timed sleep; USB enumerated later than the fixed reconnect deadline, then follow-up diagnostics passed. An initial pre-correction startup hit a touch I2C NACK and rebooted; the corrected build started cleanly.

Installed application SHA-256: 3e33f9455343eb87141b21bd6c0f5a0a9add409c09971dd5336e37d64b853f5a. Build version text is the cached 2d02f7d-dirty label; binary readback hash is the installation identity.

## Physical invalid-calendar test, 2026-10-29

SOURCE_CONFIRMED: The user requested deliberate invalid time followed by sync. Added USB-only `clock invalidate`: commit an unknown save timestamp before writing RTC year 2000, preserve oscillator/control bits and leave offline time untrusted. A failed save prevents the hardware write; a failed hardware write leaves time untrusted. Production RTC tests cover invalidation, boot rejection, subsequent recovery, absent device, failed save and failed write.

After fresh backup and application-only flash/readback, aquarium journal/rescue/meta/undo and NVS/bootloader/partition bytes were unchanged. The live command reported raw year 00, trusted=0, valid=0, clock_valid=0. `clock retry` then connected, accepted network time and committed current UTC in about nine seconds. Final diagnostics: trusted=1, OS=0, STOP=0, valid=1; configured=1, attempt_active=0. Balance 1672 and lifetime earned 2952 were unchanged across the test, and the transaction reported no offline catch-up. Memory at sample: 47,907 internal bytes free, largest 38,912, minimum 45,627; after shutdown 74,999 free. The network-corrected clock agreed with host UTC within the measurement window.

This physically verifies recovery from an invalid RTC calendar via retry. Automatic invalid-clock boot triggering and actual oscillator-stop after power depletion are separate scenarios, not established by this test. No power depletion was induced. All existing work remains preserved.

Current installed binary SHA-256: 0fa4ab15295c1113016cc77f638cfb61a8c94108215b075b54125dc0e05fd536 (1,786,688 bytes). This supersedes the earlier installed hash above.
