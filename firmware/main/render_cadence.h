#ifndef RENDER_CADENCE_H
#define RENDER_CADENCE_H
#include <stdbool.h>
#include <stdint.h>
/* A ceiling, not a claim that panel composition/transfer can achieve 30 fps. */
static inline int64_t render_interval_us(bool active,bool night){return active?33333:night?200000:125000;}
static inline bool render_due(int64_t now,int64_t last,bool active,bool night){return !last||now-last>=render_interval_us(active,night);}
#endif
