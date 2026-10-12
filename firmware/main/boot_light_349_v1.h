#ifndef BOOT_LIGHT_349_V1_H
#define BOOT_LIGHT_349_V1_H
#include <stdbool.h>
#include <stdint.h>
#include "tank.h"
enum { BOOT349_NONE, BOOT349_LIGHT, BOOT349_RESET, BOOT349_PAGES };
#define BOOT349_LONG_US 700000
typedef struct {
    bool armed, down, releasing, consumed;
    int64_t press_at, release_at;
} boot349_button_t;
/* No double gesture. Short release acts after 30ms debounce; long fires once.
 * Ignore boot-held keys and consume protected-page or reset-chord presses. */
static inline int boot349_poll(boot349_button_t *s,int64_t now,bool down,bool fresh_touch,bool confirm,bool setup) {
    if(!s->armed){if(!down)s->armed=true;return BOOT349_NONE;}
    if(down){
        if(!s->down){s->down=true;s->press_at=now;s->consumed=confirm||setup;}
        s->releasing=false;
        if(confirm||setup)s->consumed=true;
        if(!s->consumed && fresh_touch){s->consumed=true;return BOOT349_RESET;}
        if(!s->consumed && now-s->press_at>=BOOT349_LONG_US){s->consumed=true;return BOOT349_LIGHT;}
        return BOOT349_NONE;
    }
    if(!s->down)return BOOT349_NONE;
    if(confirm||setup)s->consumed=true;
    if(!s->releasing){s->releasing=true;s->release_at=now;}
    if(now-s->release_at<30000)return BOOT349_NONE;
    int64_t duration=s->release_at-s->press_at;
    bool valid=!s->consumed && duration>=50000;
    s->down=false;s->releasing=false;
    return !valid?BOOT349_NONE:duration>=BOOT349_LONG_US?BOOT349_LIGHT:BOOT349_PAGES;
}
static inline void boot349_toggle_light(tank_t *t) {
    /* Toggle what is currently visible, including AUTO night or director override.
     * MANUAL is persisted normally per slot; AUTO can be selected again in Settings. */
    t->light_manual_off=!t->night;t->light_auto=false;t->light_override=false;
    t->tap_count=0;tank_handled(t);
}
#endif
