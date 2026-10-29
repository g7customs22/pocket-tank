#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H
#include "ambient.h"
void ambient_port_init(ambient_t *a);
void ambient_port_poll(ambient_t *a,int64_t now);
bool weather_client_begin(void);
/* 0 pending, 1 complete, -1 failed; tank task only. */
int weather_client_poll(void);
void weather_client_stop(void);
#endif
