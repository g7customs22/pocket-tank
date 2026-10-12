#ifndef BOOT_LIGHT_349_V1_H
#define BOOT_LIGHT_349_V1_H
#include <stdbool.h>
#include <stdint.h>
#include "tank.h"
enum { BOOT349_NONE, BOOT349_LIGHT, BOOT349_RESET, BOOT349_PAGES, BOOT349_STYLE };
#define BOOT349_LONG_US 700000
#define BOOT349_DOUBLE_US 350000
typedef struct {
    bool armed, down, releasing, consumed, pending, second;
    int64_t press_at, release_at, single_at;
} boot349_button_t;
/* Short releases wait 350ms for a second press. Short-then-long is power only.
 * Debounce: minimum down 50ms, stable release 30ms. Boot-held keys are ignored. */
static inline int boot349_poll(boot349_button_t *s,int64_t now,bool down,bool fresh_touch,bool confirm,bool setup) {
    if(!s->armed){if(!down)s->armed=true;return BOOT349_NONE;}
    if(confirm||setup){s->pending=false;if(s->down)s->consumed=true;}
    if(!s->down&&s->pending&&now>=s->single_at){s->pending=false;return BOOT349_PAGES;}
    if(down){
        if(!s->down){s->down=true;s->press_at=now;s->consumed=confirm||setup;s->second=s->pending;}
        s->releasing=false;
        if(!s->consumed&&fresh_touch){s->consumed=true;s->pending=false;return BOOT349_RESET;}
        if(!s->consumed&&now-s->press_at>=BOOT349_LONG_US){s->consumed=true;s->pending=false;return BOOT349_LIGHT;}
        return BOOT349_NONE;
    }
    if(s->down){
        if(!s->releasing){s->releasing=true;s->release_at=now;}
        if(now-s->release_at<30000)return BOOT349_NONE;
        int64_t duration=s->release_at-s->press_at;bool valid=!s->consumed&&duration>=50000;
        s->down=false;s->releasing=false;
        if(valid&&duration>=BOOT349_LONG_US){s->pending=false;return BOOT349_LIGHT;}
        if(valid){if(s->second){s->pending=false;return BOOT349_STYLE;}
            s->pending=true;s->single_at=s->release_at+BOOT349_DOUBLE_US;}
    }
    if(s->pending&&now>=s->single_at){s->pending=false;return BOOT349_PAGES;}
    return BOOT349_NONE;
}
static inline void boot349_toggle_style(tank_t *t){t->uv.style=t->uv.style?0:1;}
static inline void boot349_toggle_light(tank_t *t) {
    /* Toggle what is currently visible, including AUTO night or director override.
     * MANUAL is persisted normally per slot; AUTO can be selected again in Settings. */
    t->light_manual_off=!t->night;t->light_auto=false;t->light_override=false;
    t->tap_count=0;tank_handled(t);
}
#endif
