#ifndef BOOT_LIGHT_349_V1_H
#define BOOT_LIGHT_349_V1_H
#include <stdbool.h>
#include <stdint.h>
#include "tank.h"
enum { BOOT349_NONE, BOOT349_LIGHT, BOOT349_RESET, BOOT349_PAGES, BOOT349_STYLE };
#define BOOT349_LONG_US 700000
#define BOOT349_DOUBLE_US 500000
typedef struct {
    bool armed, down, releasing, consumed, pending, second;
    int64_t press_at, release_at, single_at;
} boot349_button_t;
/* Window is first release to second press, inclusive. A second short press
 * may finish after the window. Stable release 30ms; minimum press 50ms.
 * Call for every timestamped edge and once at the current time, in order. */
static inline int boot349_poll(boot349_button_t *s,int64_t now,bool down,bool fresh_touch,bool confirm,bool setup) {
    int action=BOOT349_NONE;
    if(!s->armed){if(!down)s->armed=true;return action;}
    if(confirm||setup){s->pending=false;if(s->down)s->consumed=true;}
    /* Settle a previous release BEFORE accepting a new down edge. Rendering
     * may have prevented an idle poll during this otherwise stable gap. */
    if(s->down && s->releasing && now-s->release_at>=30000){
        int64_t duration=s->release_at-s->press_at;
        bool valid=!s->consumed&&duration>=50000;
        s->down=false;s->releasing=false;
        if(valid&&duration>=BOOT349_LONG_US){s->pending=false;action=BOOT349_LIGHT;}
        else if(valid){
            if(s->second){s->pending=false;action=BOOT349_STYLE;}
            else {s->pending=true;s->single_at=s->release_at+BOOT349_DOUBLE_US;}
        }
    }
    if(!s->down&&s->pending&&(now>s->single_at||(!down&&now==s->single_at))){
        s->pending=false;action=BOOT349_PAGES;
    }
    if(down){
        if(!s->down){s->down=true;s->press_at=now;s->consumed=confirm||setup;s->second=s->pending;}
        s->releasing=false;
        if(!s->consumed&&fresh_touch){s->consumed=true;s->pending=false;return BOOT349_RESET;}
        if(!s->consumed&&now-s->press_at>=BOOT349_LONG_US){s->consumed=true;s->pending=false;return BOOT349_LIGHT;}
    }else if(s->down&&!s->releasing){s->releasing=true;s->release_at=now;}
    return action;
}
/* The ESP timer only records GPIO transitions. The tank task owns recognition
 * and all UI/game mutations. Overflow discards the batch and disarms the key. */
#define BOOT349_EDGE_CAP 64
#define BOOT349_SAMPLE_US 5000
typedef struct {int64_t at;bool down;} boot349_edge_t;
typedef struct {boot349_edge_t edges[BOOT349_EDGE_CAP];unsigned count;bool raw_down,overflow;} boot349_capture_t;
static inline void boot349_capture(boot349_capture_t *c,int64_t now,bool down){
    if(down==c->raw_down)return;
    c->raw_down=down;
    if(c->count==BOOT349_EDGE_CAP)c->overflow=true;
    else c->edges[c->count++]=(boot349_edge_t){now,down};
}
static inline void boot349_toggle_style(tank_t *t){t->uv.style=t->uv.style?0:1;}
static inline void boot349_toggle_light(tank_t *t) {
    /* Toggle what is currently visible, including AUTO night or director override.
     * MANUAL is persisted normally per slot; AUTO can be selected again in Settings. */
    t->light_manual_off=!t->night;t->light_auto=false;t->light_override=false;
    t->tap_count=0;tank_handled(t);
}
#endif
