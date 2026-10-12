#ifndef BOOT_LIGHT_349_V1_H
#define BOOT_LIGHT_349_V1_H
#include <stdbool.h>
#include <stdint.h>
#include "tank.h"
enum { BOOT349_NONE, BOOT349_LIGHT, BOOT349_RESET };
typedef struct {
    bool armed, down, releasing, consumed;
    int64_t press_at, release_at;
} boot349_button_t;
/* GPIO0 remains an input/boot strap. Ignore a key held through boot; act once
 * after a debounced release. A reset chord or protected page consumes the press. */
static inline int boot349_poll(boot349_button_t *s,int64_t now,bool down,bool fresh_touch,bool confirm,bool setup) {
    if(!s->armed){if(!down)s->armed=true;return BOOT349_NONE;}
    if(down){
        if(!s->down){s->down=true;s->press_at=now;s->consumed=false;}
        s->releasing=false;
        if(!s->consumed && fresh_touch && !confirm){s->consumed=true;return BOOT349_RESET;}
        if(confirm||setup)s->consumed=true;
        return BOOT349_NONE;
    }
    if(!s->down)return BOOT349_NONE;
    if(confirm||setup)s->consumed=true;
    if(!s->releasing){s->releasing=true;s->release_at=now;}
    if(now-s->release_at<30000)return BOOT349_NONE;
    int64_t duration=s->release_at-s->press_at;
    bool toggle=!s->consumed && duration>=50000 && duration<700000;
    s->down=false;s->releasing=false;
    return toggle?BOOT349_LIGHT:BOOT349_NONE;
}
static inline void boot349_toggle_light(tank_t *t) {
    /* Toggle what is currently visible, including AUTO night or director override.
     * MANUAL is persisted normally per slot; AUTO can be selected again in Settings. */
    t->light_manual_off=!t->night;t->light_auto=false;t->light_override=false;
    t->tap_count=0;tank_handled(t);
}
#endif
