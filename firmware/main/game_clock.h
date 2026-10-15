#ifndef GAME_CLOCK_H
#define GAME_CLOCK_H
#include <stdbool.h>
#include <stdint.h>
#define GAME_CLOCK_SLOTS 3
#define GAME_CLOCK_BATCH 8
#define GAME_CLOCK_STEP (1.0f/30.0f)
/* Monotonic awake-time debt, in microseconds * 30. No RTC or save-format change.
 * Integer accounting avoids dropping a fractional step on every slow frame. */
typedef struct { int64_t last_us, debt[GAME_CLOCK_SLOTS]; unsigned slot; } game_clock_t;
static inline void game_clock_init(game_clock_t *c,int64_t now,unsigned slot){*c=(game_clock_t){.last_us=now,.slot=slot<GAME_CLOCK_SLOTS?slot:0};}
static inline void game_clock_account(game_clock_t *c,int64_t now){if(now<=c->last_us)return;c->debt[c->slot]+=(now-c->last_us)*30;c->last_us=now;}
static inline bool game_clock_due(const game_clock_t *c){return c->debt[c->slot]>=1000000;}
static inline unsigned game_clock_take(game_clock_t *c){unsigned n=0;while(n<GAME_CLOCK_BATCH&&game_clock_due(c)){c->debt[c->slot]-=1000000;n++;}return n;}
/* Called after intentional sleep or transition I/O, never for slow rendering.
 * The old slot keeps its previously earned remainder; inactive time is absent. */
static inline void game_clock_resume(game_clock_t *c,int64_t now,unsigned slot){c->last_us=now;c->slot=slot<GAME_CLOCK_SLOTS?slot:0;}
static inline void game_clock_reset_active(game_clock_t *c,int64_t now){c->debt[c->slot]=0;c->last_us=now;}
#endif
