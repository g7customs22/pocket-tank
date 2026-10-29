#ifndef TIME_RECOVERY_H
#define TIME_RECOVERY_H
#include <stdbool.h>
/* All calls except the SNTP/event callbacks run on the tank task. */
void time_recovery_init(void);
void time_recovery_poll(void);
void time_recovery_stop(void);
bool time_recovery_configure(const char *ssid_hex, const char *password_hex);
void time_recovery_retry(void);
void time_recovery_sync(void);
bool time_recovery_weather(void);
void time_recovery_status(void);
#endif
