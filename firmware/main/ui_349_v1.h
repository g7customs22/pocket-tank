/* V1 presentation only. The 448x368 simulation and its hit geometry stay intact. */
#ifndef UI_349_V1_H
#define UI_349_V1_H
#include "tank.h"
#ifdef POCKET_WORLD_PROTO
#include "world_proto.h"
#endif
#define UI349_W 640
#define UI349_H 172
#define UI349_PAGE_X 96
#define UI349_SCROLL_STEP 40 /* two 20px text lines; selection navigation is unchanged */
#define UI349_SCROLL_MAX (TANK_H - UI349_H)
typedef struct {
    int key;                 /* zero = aquarium, nonzero = readable page */
    const char *title;
    bool can_close;
    int initial_scroll;
} ui349_view_t;
/* Rough 1S 4.2V Li-ion voltage estimate, NOT a fuel gauge. USB/charging,
 * load and cell chemistry shift it. Conservative generic knee/plateau anchors;
 * no runtime, temperature or current model is claimed. See THREE_TANKS.md. */
typedef struct { int mv, pct; bool low, warned; unsigned low_samples, recover_samples, warning_s; } ui349_battery_t;
static inline int ui349_battery_estimate(int mv) {
    static const int volts[]={3300,3500,3600,3700,3800,3900,4000,4100,4200};
    static const int pct[]={0,5,10,20,40,60,75,90,100};
    if(mv<=volts[0])return 0;
    if(mv>=4200)return 100;
    for(int i=1;i<9;i++)if(mv<=volts[i])return pct[i-1]+(mv-volts[i-1])*(pct[i]-pct[i-1])/(volts[i]-volts[i-1]);
    return 100;
}
static inline void ui349_battery_update(ui349_battery_t *b,int mv) {
    if(b->warning_s)b->warning_s--;
    if(mv<2500||mv>4350){b->low_samples=b->recover_samples=0;b->warning_s=0;b->mv=0;b->pct=-1;b->low=false;return;}
    bool first=!b->mv;b->mv=first?mv:(3*b->mv+mv+2)/4;
    int pct=ui349_battery_estimate(b->mv);
    if(first||pct<=b->pct-3||pct>=b->pct+3)b->pct=pct;
    b->low=b->pct<=10;
    b->low_samples=b->low?b->low_samples+1:0;
    b->recover_samples=b->pct>=15?b->recover_samples+1:0;
    if(b->recover_samples>=3){b->warned=false;b->recover_samples=3;}
    if(b->low_samples>=3){b->low_samples=3;if(!b->warned){b->warning_s=5;b->warned=true;}}

}
typedef struct {
#ifdef POCKET_WORLD_PROTO
    wp_camera_t world_camera;
    bool world_ready,copy_picker,copy_placing,copy_error;
    int copy_item,copy_selected,copy_depth;
#endif
    ui349_battery_t battery;
    ui349_view_t view;
    int camera_x, camera_y;
    int scroll, owner, pressed, press_key, last_x, last_y, press_x, press_y;
    bool positioning, rail_moved, species_picker, habitat_picker;
    bool controls_hidden; /* presentation only; clean aquarium has no control hitboxes */
    int habitat, species_page;
    bool slots_picker, slot_error, can_undo;
    unsigned feed_notice_s; int slot_fish[3];
    unsigned active_slot, slots_present;
} ui349_state_t;
/* First-sample presses. Short contacts need a prompt release so two taps do
 * not become one hold. Established holds/drags retain the 50 ms dropout bridge. */
typedef struct {
    bool held, moving;
    int64_t last_down, press_at;
    uint16_t x,y,press_x,press_y;
    unsigned accepted, rejected;
} ui349_contact_t;
static inline void ui349_contact(ui349_contact_t *s, int64_t now, bool *down, uint16_t *x, uint16_t *y) {
    if(*down) {
        if(!s->held) {
            s->accepted++;s->press_at=now;s->press_x=*x;s->press_y=*y;s->moving=false;
        }
        int dx=(int)*x-s->press_x,dy=(int)*y-s->press_y;
        if(dx*dx+dy*dy>=8*8)s->moving=true;
        s->held=true;s->last_down=now;s->x=*x;s->y=*y;
    } else {
        int64_t bridge=s->moving || s->last_down-s->press_at>=300000 ? 50000 : 12000;
        if(s->held && now-s->last_down<bridge) {*down=true;*x=s->x;*y=s->y;}
        else s->held=false;
    }
}

enum { UI349_COPIES=400,UI349_COPY_BACK,UI349_COPY_PREV,UI349_COPY_NEXT,UI349_COPY_BUY,UI349_COPY_MOVE,UI349_COPY_DEPTH,UI349_COPY_SELL,UI349_COPY_DONE };
enum { UI349_PASS, UI349_CONSUMED, UI349_MENU, UI349_SETTINGS, UI349_SHOP, UI349_CLOSE, UI349_NEXT, UI349_SPECIES, UI349_SPECIES_BACK, UI349_HABITAT=20, UI349_HABITAT_BACK, UI349_TANKS, UI349_TANKS_BACK, UI349_TANK_STYLE, UI349_SPECIES_PREV, UI349_SPECIES_NEXT, UI349_UNDO, UI349_SLOT_PICK=300, UI349_SPECIES_PICK=100, UI349_HABITAT_PICK=200 };
/* Active tank taps are consumed by the rail, but never dispatch a switch. */
static inline int ui349_slot_request(const ui349_state_t *s,int action){
    int slot=action-UI349_SLOT_PICK;
    return slot>=0&&slot<3&&(unsigned)slot!=s->active_slot?slot:-1;
}
/* Classify movement in screen pixels, independent of aquarium scale. */
static inline bool ui349_tap(bool world, bool modal, int64_t elapsed_us, float dx, float dy) {
    #ifndef POCKET_WORLD_PROTO
    if (world) { dx *= (float)(TANK_W * UI349_H / TANK_H - 1) / (TANK_W - 1); dy *= (float)(UI349_H - 1) / (TANK_H - 1); }
#else
    (void)world;
#endif
    return elapsed_us < (modal ? 800000 : 350000) && dx*dx + dy*dy < 24*24;
}
static inline bool ui349_clean_view(const ui349_state_t *s){return s->controls_hidden && s->view.key==0 && !s->positioning;}
bool ui349_world_view(const ui349_state_t *s);
void ui349_configure(ui349_state_t *s, ui349_view_t view);
bool ui349_map(const ui349_state_t *s, int x, int y, int *tx, int *ty);
int ui349_input(ui349_state_t *s, bool down, int x, int y, bool *content_down, int *tx, int *ty);
void ui349_render(const ui349_state_t *s, const tank_t *t, const uint16_t *source, uint16_t *native);
/* Firmware-owned state, also used by the touch adapter. */
extern ui349_state_t g_ui349;
void ui349_capture_fish(ui349_state_t *s, const tank_t *tank, const uint16_t *source, uint16_t *native);
void display349_view(tank_t *tank, ui349_view_t view, const uint16_t *source);
#endif
