/* touch_port_ft3168.c — FT3168 capacitive touch (FT5x06 register family) ->
 * tank_touch_hold / tank_touch_tap, with the same gesture timing as the sim's
 * mouse: press+release < 350 ms with < 24 px displacement = tap (fingertips
 * roll and this panel is 322 ppi); held > 300 ms = hold; a drag down from
 * the top edge = feed at that x; every touched frame streams to
 * tank_touch_drag (a moving stroke wipes algae; a horizontal slash through
 * a canopy trims it). Fish taps hit-test 38 px against the press-time fish
 * snapshot AND the current position - fish move during a tap. While the stats
 * card is up, a tap anywhere on empty glass dismisses it (hunting the same
 * fish again to close it was the old, cumbersome way) and does nothing else.
 * While the battery pill shows (main.c says so each frame), a tap on it -
 * RENDER_BAT_HIT, the pill plus a fingertip's slop - opens the battery page;
 * any release closes that page (2026-09-24).
 * Coordinates are mapped from the portrait panel to the landscape tank. */
#include "touch_port.h"
#include "board_pins.h"
#include "tank.h"
#include "render.h"
#include "setup.h"
#include "notice.h"
#include "audio_port.h"
#include "progression.h"
#include "sdkconfig.h"
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
#include "board_waveshare_349_v1.h"
#include "viewport_349_v1.h"
#include "ui_349_v1.h"
#else
#include "esp_lcd_touch_ft5x06.h"
#include "esp_lcd_touch_cst816s.h"
#endif
#include "esp_lcd_panel_io.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include <math.h>

static const char *TAG = "touch";
#ifndef CONFIG_POCKET_TANK_WAVESHARE_349_V1
static esp_lcd_touch_handle_t s_tp;
#endif
static bool s_down; static int64_t s_press_us; static float s_px, s_py;
static float s_lx, s_ly;                          /* LAST touched position (release classification) */
static float s_fx[N_FISH_MAX], s_fy[N_FISH_MAX];  /* fish positions at press time */
static int s_sel = -1; static int64_t s_sel_us;   /* tapped fish -> stats card */
static bool s_ms;                                 /* milestones page up (its CLOSE button ends it) */
static bool s_cf; static int64_t s_cf_us; static int s_cf_ans;   /* reset confirm prompt */
static bool s_set;                                /* settings page up (CLOSE returns to the milestones page) */
static bool s_shop;                               /* the shop page up (CLOSE returns to the milestones page) */
static bool s_back;                               /* the settings page's CLOSE just brought the milestones page back: that
                                                     release must not reach the page as a tap on ITS CLOSE (same spot) */
static int  s_shop_act;                           /* an UNLOCK / MOVE / SELL tapped: the raw tap code, for main (one-shot) */
static bool s_held_page;                          /* this press opened a piece's page by holding on it (2026-09-24) */
static int  s_set_what, s_set_val;                /* a segment tapped: SET_TAP_* + value, for main */
static bool s_pill;                               /* main.c drew the battery pill this frame: a tap on it is its page's */
static bool s_bat; static int64_t s_bat_us;       /* the battery page up, and since when (it closes itself) */
#define BATTERY_PAGE_US (30LL * 1000000)
#define CONFIRM_TIMEOUT_US (20LL * 1000000)
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
static int s_slot_request=-1;
static struct { bool valid,ms,set,shop,bat; int sel,scroll,species_page; bool species,slots,habitat; ui349_view_t view; } s_hidden;
static bool s_press_world;
static bool s_trimmed; /* Latch across simulation ticks until release. */
#endif
static bool s_inverted;                           /* screen 180-flipped: mirror into tank space */
/* Fingers land a little BELOW where the eye aims - the pad rolls onto the
 * glass under the fingertip (phones shift their hit targets down for the
 * same reason; Strato saw it on the swatch rows, 2026-09-13). Reported
 * points move UP by this many px in displayed space; director `touch bias
 * <px>` tunes it live. */
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
static int s_bias_y; /* The original panel's finger bias is not calibrated here. */
#else
static int s_bias_y = 10;
#endif
void touch_port_set_bias(int px) { s_bias_y = px; }
int  touch_port_bias(void) { return s_bias_y; }

void touch_port_set_inverted(bool inverted) { s_inverted = inverted; }
extern i2c_master_bus_handle_t board_i2c_bus(void);
extern bool board_is_v2(void);

bool touch_port_init(void) {
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    bool down; uint16_t x, y;
    esp_err_t err = board_349_v1_touch(&down, &x, &y);
    ESP_LOGI(TAG, "V1 touch startup: %s", esp_err_to_name(err));
    return err == ESP_OK;
#else
    esp_lcd_panel_io_handle_t io;
    bool v2 = board_is_v2();
    esp_lcd_panel_io_i2c_config_t io_cfg = v2 ? (esp_lcd_panel_io_i2c_config_t)ESP_LCD_TOUCH_IO_I2C_CST816S_CONFIG()
                                              : (esp_lcd_panel_io_i2c_config_t)ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    io_cfg.dev_addr = v2 ? I2C_ADDR_CST816 : I2C_ADDR_FT3168; io_cfg.scl_speed_hz = 400000;
    if (esp_lcd_new_panel_io_i2c(board_i2c_bus(), &io_cfg, &io) != ESP_OK) { ESP_LOGW(TAG, "no touch io"); return false; }
    esp_lcd_touch_config_t tp_cfg = { .x_max = PANEL_W, .y_max = PANEL_H, .rst_gpio_num = -1, .int_gpio_num = -1,
        .levels = { .reset = 0, .interrupt = 0 }, .flags = { .swap_xy = 0, .mirror_x = 0, .mirror_y = 0 } };
    esp_err_t err = v2 ? esp_lcd_touch_new_i2c_cst816s(io, &tp_cfg, &s_tp)
                       : esp_lcd_touch_new_i2c_ft5x06(io, &tp_cfg, &s_tp);
    if (err != ESP_OK) { ESP_LOGW(TAG, "no %s", v2 ? "CST816" : "FT3168"); return false; }
    ESP_LOGI(TAG, "%s ready", v2 ? "CST816" : "FT3168");
    return true;
#endif
}

/* call every frame from the tank task */
void touch_port_poll(tank_t *t) {
    int64_t now = esp_timer_get_time();
    if (s_cf && now - s_cf_us > CONFIRM_TIMEOUT_US) touch_port_confirm_answer(-1);   /* nobody answered: keep the tank */
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    static int64_t last_poll, report_at;
    static unsigned reads, errors, timing_reads;
    static int64_t read_us,max_read_us,max_gap_us;
    static ui349_contact_t contact;
    if(now-last_poll<8000) return;
    if(last_poll&&now-last_poll>max_gap_us)max_gap_us=now-last_poll;
    last_poll=now;
    bool touched; uint16_t raw_x, raw_y;
    reads++;
    esp_err_t touch_err=board_349_v1_touch(&touched,&raw_x,&raw_y);
    int64_t elapsed=esp_timer_get_time()-now;read_us+=elapsed;timing_reads++;
    if(elapsed>max_read_us)max_read_us=elapsed;
    if(touch_err!=ESP_OK)errors++;
    if(now-report_at>=10000000) {
        ESP_LOGI(TAG,"filter reads=%u accepted=%u rejected=%u errors=%u",reads,contact.accepted,contact.rejected,errors);
        ESP_LOGI(TAG,"timing ms: read avg %.2f max %.2f poll-gap max %.2f (%u reads)",
          read_us/1000.f/timing_reads,max_read_us/1000.f,max_gap_us/1000.f,timing_reads);
        read_us=max_read_us=max_gap_us=0;timing_reads=0;report_at=now;
    }
    if(touch_err!=ESP_OK)return;
    ui349_contact(&contact, now, &touched, &raw_x, &raw_y);
    int mapped_x, mapped_y;
    int ux = raw_x, uy = raw_y;
    if (s_inverted) { ux = UI349_W - 1 - ux; uy = UI349_H - 1 - uy; }
    int action = ui349_input(&g_ui349, touched, ux, uy, &touched, &mapped_x, &mapped_y);
    if (action != UI349_PASS) {
        if(action!=UI349_CONSUMED) ESP_LOGI(TAG,"navigation action=%d at %d,%d",action,ux,uy);
        if (action == UI349_MENU || action == UI349_SETTINGS || action == UI349_SHOP) {
            touch_port_dismiss(); notice_dismiss();
            if (action == UI349_MENU) touch_port_show_milestones(true);
            else if (action == UI349_SETTINGS) touch_port_show_settings(true);
            else touch_port_show_shop(true);
        }
        else if (action == UI349_SPECIES && s_sel>=0 && s_sel<t->n_fish) {
            g_ui349.species_picker=true; g_ui349.species_page=0; g_ui349.habitat=t->habitat; s_sel_us=now;
        } else if(action==UI349_SPECIES_BACK) { g_ui349.species_picker=false; s_sel_us=now; }
        else if(action==UI349_SPECIES_PREV){if(g_ui349.species_page>0)g_ui349.species_page--;s_sel_us=now;}
        else if(action==UI349_SPECIES_NEXT){int n=0;for(int i=0;i<SPECIES_COUNT;i++)if(tank_species_allowed(t->habitat,i))n++;if((g_ui349.species_page+1)*6<n)g_ui349.species_page++;s_sel_us=now;}
        else if(action>=UI349_SPECIES_PICK && action<UI349_SPECIES_PICK+SPECIES_COUNT) {
            if(tank_species_allowed(t->habitat,action-UI349_SPECIES_PICK) && tank_set_species(t,s_sel,action-UI349_SPECIES_PICK)) {
                progression_save(t); s_sel_us=now;
                ESP_LOGI(TAG,"species: %s -> %s",t->fish[s_sel].name,TANK_SPECIES[action-UI349_SPECIES_PICK].name);
            }
            g_ui349.species_picker=false;
        }
        else if(action==UI349_TANKS) {g_ui349.slots_picker=true;g_ui349.slot_error=false;}
        else if(action==UI349_UNDO)s_slot_request=-2;
        else if(action==UI349_TANKS_BACK) g_ui349.slots_picker=false;
        else if(action==UI349_TANK_STYLE) {g_ui349.slots_picker=false;g_ui349.habitat_picker=true;}
        else if(action>=UI349_SLOT_PICK&&action<UI349_SLOT_PICK+3) s_slot_request=action-UI349_SLOT_PICK;
        else if(action==UI349_HABITAT) g_ui349.habitat_picker=true;
        else if(action==UI349_HABITAT_BACK) g_ui349.habitat_picker=false;
        else if(action>=UI349_HABITAT_PICK && action<UI349_HABITAT_PICK+HABITAT_COUNT) {
            t->habitat=action-UI349_HABITAT_PICK;
            progression_save(t);g_ui349.habitat_picker=false;
            ESP_LOGI(TAG,"habitat: %s",HABITAT_NAMES[t->habitat]);
        }
        else if (action == UI349_NEXT && setup_active()) setup_activate(t, SETUP_HIT_NEXT);
        else if (action == UI349_CLOSE && !setup_active() && !touch_port_confirm_up()) {
            if (s_ms) progression_ack_milestones(t);
            touch_port_dismiss(); notice_dismiss();
        }
        return;
    }
    if (g_ui349.positioning) mapped_y = TANK_H - 4; /* placement's drag zone, never its menu hit boxes */
    float tx = touched ? mapped_x : s_lx;
    float ty = touched ? mapped_y - s_bias_y : s_ly;
#else
    if (!s_tp) return;
    uint16_t x[1], y[1], st[1]; uint8_t n = 0;
    esp_lcd_touch_read_data(s_tp);
    bool touched = esp_lcd_touch_get_coordinates(s_tp, x, y, st, &n, 1) && n > 0;
    /* portrait panel (px,py) -> landscape tank (tx,ty): tx = TANK_W-1-py, ty = px;
     * flipped screen: mirror both, so downstream gestures live in displayed space */
    float tx = touched ? (s_inverted ? (float)y[0] : (float)(TANK_W - 1 - y[0])) : s_lx;
    float ty = touched ? (s_inverted ? (float)(TANK_H - 1 - x[0]) : (float)x[0]) - s_bias_y : s_ly;
#endif
    if (touched && ty < 0) ty = 0;
    if (touched && !s_down) {
        audio_port_prewarm();                   /* the release's cue plays warm */
        s_press_us = now; s_px = tx; s_py = ty;
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
        s_press_world = ui349_world_view(&g_ui349);
        s_trimmed = false;
#endif
        /* snapshot the school: the user aims at where a fish WAS - by release
           a darting fish has moved and the finger hid it the whole time */
        for (int i = 0; i < t->n_fish && i < N_FISH_MAX; i++) { s_fx[i] = t->fish[i].x; s_fy[i] = t->fish[i].y; }
    }
    bool su = setup_active();                                /* before the touch: BEGIN's release is not a tank tap */
    if (s_set && !s_cf && !su) {                             /* the settings page owns the glass: segments, the seconds wheel, CLOSE */
        int v = 0, r = render_settings_touch(t, tx, ty, touched, &v);
        if (r) ESP_LOGI(TAG, "settings: %s %d", r == SET_TAP_CLOSE ? "CLOSE" : r == SET_TAP_BRIGHT ? "brightness" : r == SET_TAP_VOLUME ? "volume"
                                                  : r == SET_TAP_LIGHT ? "lights out" : "idle seconds", v);
        if (r == SET_TAP_CLOSE) { s_set = false; s_ms = true; s_back = true; }   /* back to the milestones page (2026-09-16); the release is spent */
        else if (r == SET_TAP_BRIGHT || r == SET_TAP_VOLUME || r == SET_TAP_LIGHT || r == SET_TAP_IDLE) { s_set_what = r; s_set_val = v; }
    }
    if (su && !s_cf) {
        bool birth = setup_is_birth(); int who = setup_fish(), place = setup_item();
        setup_touch(t, tx, ty, touched);                     /* taps and the letter wheel, classified in setup.c */
        if (!setup_active()) {
            if (birth) ESP_LOGI(TAG, "birth flow done: %s named and saved", who >= 0 && who < t->n_fish ? t->fish[who].name : "?");
            else if (place >= 0) ESP_LOGI(TAG, "placed: %s at x %.0f, %s layer, saved", SD_ITEMS[place].name, tank_decor_x(t, place),
                                          tank_decor_z(t, place) == DECOR_Z_BACK ? "BEHIND" : tank_decor_z(t, place) == DECOR_Z_FRONT ? "IN FRONT" : "AMONG");
            else ESP_LOGI(TAG, "setup done: %s + %s", t->fish[0].name, t->fish[1].name);
        }
    }
    bool clean=false;
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    clean=ui349_clean_view(&g_ui349) && !su && !s_cf;
#endif
    bool modal = s_ms || s_set || s_shop || s_cf || su || s_bat;      /* a page or a prompt owns the glass */
    if (touched) { s_lx = tx; s_ly = ty; if (!modal) tank_touch_drag(t, tx, ty); }  /* stroke = wipe/slash */
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    if (touched && !modal && t->slash_engaged) s_trimmed = true;
#endif
    /* tap-and-hold on a decoration (2026-09-24, Strato: "right now it's like 5
       taps to get back to the edit"): a still finger 700 ms on an owned
       piece opens its placement page (MOVE / DEPTH / SELL) right there */
    if (!clean && touched && !modal && !s_held_page && now - s_press_us > 700000 && fabsf(tx - s_px) < 24 && fabsf(ty - s_py) < 24) {
        int it = tank_decor_hit(t, s_px, s_py);
        if (it >= 0) { setup_begin_place(t, it); s_held_page = true; s_sel = -1;
                       ESP_LOGI(TAG, "held on the %s: placement page up", SD_ITEMS[it].name); }
    }
    if (touched && !modal && !s_held_page && now - s_press_us > 300000 && fabsf(ty - s_py) < 30) tank_touch_hold(t, tx, ty);
    if (!touched && s_down) {
        /* release: classify with the LAST touched position (the old code fell
           back to the PRESS position here, so dx/dy were always 0 - every
           quick swipe read as a tap and the drag-feed could never fire) */
        float dx = s_lx - s_px, dy = s_ly - s_py;
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
        if (s_trimmed) goto released; /* Scissors cannot also tap or drag-feed. */
#endif
        if (s_cf) {                     /* the prompt owns the glass: a press AND release on the
                                           same button answers it, nothing else counts - not
                                           even the tap that opened it (it began before) */
            int h = s_press_us > s_cf_us ? render_confirm_hit(s_px, s_py) : 0;
            if (h && h == render_confirm_hit(s_lx, s_ly)) touch_port_confirm_answer(h);
            goto released;
        }
        if (su) {                       /* the setup had the glass (setup_touch above); just the log:
                                           where the finger landed vs what it hit, in case this panel
                                           reports fingers offset from where they feel */
            ESP_LOGI(TAG, "setup touch press %.0f,%.0f release %.0f,%.0f -> %s", s_px, s_py, s_lx, s_ly,
                     setup_hit_name(setup_active() ? setup_hit(s_px, s_py) : 0));
            s_sel = -1; goto released;
        }
        if (s_bat) { s_bat = false; ESP_LOGI(TAG, "battery page closed"); goto released; }   /* any release closes it */
        #ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
        if (ui349_tap(s_press_world, modal, now - s_press_us, dx, dy)) {
#else
        if (now - s_press_us < 350000 && dx * dx + dy * dy < 24 * 24) {
#endif
            if(clean){tank_touch_tap(t,s_px,s_py);goto released;}
            if (notice_current()) { notice_dismiss(); ESP_LOGI(TAG, "tap closed the announcement"); goto released; }
            if (s_set || s_back) { s_back = false; goto released; }   /* the settings page had the glass (render_settings_touch above) */
            if (s_shop) {                                           /* the shop: a row's modal, UNLOCK, HOW TO EARN, CLOSE */
                int r = render_shop_tap(t, s_px, s_py);
                ESP_LOGI(TAG, "shop tap at %.0f,%.0f -> %s", s_px, s_py, r == SHOP_TAP_CLOSE ? "CLOSE" : r >= SHOP_TAP_SELL ? "SELL" : r >= SHOP_TAP_MOVE ? "MOVE" : r >= SHOP_TAP_BUY ? "UNLOCK" : r == SHOP_TAP_KEPT ? "modal" : "nothing");
                if (r == SHOP_TAP_CLOSE) { s_shop = false; render_shop_leave(); s_ms = true; }   /* back to the milestones page (2026-09-16) */
                else if (r >= SHOP_TAP_BUY) s_shop_act = r;   /* main.c buys (and plays the cue) or opens the placement page */
                goto released;
            }
            if (s_ms) {                                             /* the page: badges open a modal, the CLOSE
                                                                       button ends it, the brightness row cycles */
                int r = render_milestones_tap(t, s_px, s_py);     /* CLOSE / SETTINGS / the sand dollar, detail modal, nothing */
                ESP_LOGI(TAG, "page tap at %.0f,%.0f (release %.0f,%.0f) -> %s", s_px, s_py, s_lx, s_ly,
                         r == MS_TAP_CLOSE ? "CLOSE" : r == MS_TAP_SETTINGS ? "SETTINGS" : r == MS_TAP_SHOP ? "SHOP" : r == MS_TAP_KEPT ? "detail" : "nothing");
                if (r != MS_TAP_CLOSE && r != MS_TAP_SETTINGS && r != MS_TAP_SHOP) goto released;   /* only a button leaves the page */
                s_ms = false; s_sel = -1; s_set = r == MS_TAP_SETTINGS; s_shop = r == MS_TAP_SHOP;
                progression_ack_milestones(t); render_milestones_leave();   /* everything shown is now "seen" */
                goto released;
            }
            if (s_pill && RENDER_BAT_HIT(s_px, s_py)) {      /* the battery pill, while it shows: its page (2026-09-24) */
                s_bat = true; s_bat_us = now; s_sel = -1;
                ESP_LOGI(TAG, "battery pill tapped at %.0f,%.0f: battery page up", s_px, s_py);
                goto released;
            }
            if (s_sel >= 0 && s_sel != RENDER_CARD_SNAIL && s_sel != RENDER_CARD_SHRIMP && RENDER_CARD_HIT(s_px, s_py)) {   /* a tap ON the card (or the slop
                ESP_LOGI(TAG, "card tap at %.0f,%.0f -> milestones", s_px, s_py);         under its MORE button) = milestones page */
                s_ms = true; goto released;
            }
            /* fish first; only an empty tap reaches the water. 38 px radius
               (a fingertip on this 322 ppi panel covers ~60 px) against BOTH
               the press-time snapshot and the current position - whichever is
               closer - so a fish that moved mid-tap still registers. */
            int best = -1; float bd = 38 * 38;
            for (int i = 0; i < t->n_fish; i++) {
                float ax = s_fx[i] - s_px, ay = s_fy[i] - s_py;
                float bx = t->fish[i].x - s_px, by = t->fish[i].y - s_py;
                float d2a = ax * ax + ay * ay, d2b = bx * bx + by * by;
                float d2 = d2a < d2b ? d2a : d2b;
                if (d2 < bd) { bd = d2; best = i; }
            }
            if (best >= 0) { s_sel = (best == s_sel) ? -1 : best; s_sel_us = now; }
            else if (tank_snail_hit(t, s_px, s_py)) {   /* the snail: its card (2026-09-16), the fish first */
                s_sel = s_sel == RENDER_CARD_SNAIL ? -1 : RENDER_CARD_SNAIL; s_sel_us = now;
                ESP_LOGI(TAG, "snail tapped: card %s (%d spots grazed)", s_sel >= 0 ? "up" : "down", (int)t->snail_grazed); }
            else if (tank_shrimp_hit(t, s_px, s_py)) {  /* the shrimp school (2026-09-29): a tap its card, the third quick tap scares them */
                int taps = tank_shrimp_tap(t, s_px, s_py);
                if (taps == 1) { s_sel = s_sel == RENDER_CARD_SHRIMP ? -1 : RENDER_CARD_SHRIMP; s_sel_us = now; }
                else if (taps >= 3) s_sel = -1;
                ESP_LOGI(TAG, "shrimp tapped (%d): card %s | %d shrimp, %d eaten, %d/%d to the next", taps, s_sel == RENDER_CARD_SHRIMP ? "up" : "down",
                         t->shrimp_n, (int)t->shrimp_eaten, t->shrimp_food, SHRIMP_PER_JOIN); }
            else if (s_sel >= 0) s_sel = -1;   /* card up: a tap on empty glass just
                                                  dismisses it - it is NOT a tank tap
                                                  (no feed, no light-toggle burst) */
            else tank_touch_tap(t, s_px, s_py);
        }
        else if (!modal && s_py < 60 && dy >= 40) tank_feed(t, s_lx, 3);  /* drag down from the top = feed */
    }
released:
    if (!touched) s_held_page = false;
    s_down = touched;
    if (s_sel >= t->n_fish && s_sel != RENDER_CARD_SNAIL && s_sel != RENDER_CARD_SHRIMP) s_sel = -1;   /* fresh tank / save load */
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    if(g_ui349.species_picker && s_sel>=0) s_sel_us=now;
#endif
    if (s_sel >= 0 && now - s_sel_us > 10 * 1000000) s_sel = -1; /* auto-dismiss */
    if (s_bat && now - s_bat_us > BATTERY_PAGE_US) s_bat = false; /* the battery page too, after a while */
}

int touch_port_selected(void) { return s_sel; }
bool touch_port_milestones(void) { return s_ms; }
void touch_port_show_milestones(bool on) { if (s_ms && !on) render_milestones_leave(); s_ms = on; }
void touch_port_dismiss(void) {
#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
    if(s_hidden.valid){if(s_hidden.ms)render_milestones_leave();if(s_hidden.shop)render_shop_leave();s_hidden.valid=false;}
#endif
 s_sel = -1; if (s_ms) render_milestones_leave(); if (s_shop) render_shop_leave(); s_ms = false; s_set = false; s_shop = false; s_bat = false; }

/* ---- reset confirm prompt ---- */
void touch_port_confirm_open(void) {
    s_cf = true; s_cf_us = esp_timer_get_time(); s_cf_ans = 0;
    s_sel = -1; s_ms = false; s_set = false; s_shop = false; s_bat = false; render_shop_leave();   /* it replaces the card / the pages */
    ESP_LOGI(TAG, "reset prompt up (YES / NO on the glass; NO by itself in %d s)", (int)(CONFIRM_TIMEOUT_US / 1000000));
}
bool touch_port_confirm_answer(int ans) {
    if (!s_cf) return false;
    s_cf = false; s_cf_ans = ans > 0 ? 1 : -1;
    return true;
}
bool  touch_port_confirm_up(void)   { return s_cf; }
float touch_port_confirm_frac(void) {
    if (!s_cf) return 0;
    float f = 1.0f - (esp_timer_get_time() - s_cf_us) / (float)CONFIRM_TIMEOUT_US;
    return f < 0 ? 0 : f;
}
int  touch_port_confirm_take(void)  { int a = s_cf_ans; s_cf_ans = 0; return a; }
bool touch_port_pressed_since(int64_t us) { return s_down && s_press_us > us; }
bool touch_port_settings(void) { return s_set; }
void touch_port_show_settings(bool on) { s_set = on; if (on) { s_ms = false; s_sel = -1; s_bat = false; } }
int  touch_port_take_setting(int *value) { int w = s_set_what; *value = s_set_val; s_set_what = 0; return w; }
bool touch_port_shop(void) { return s_shop; }
void touch_port_show_shop(bool on) { if (s_shop && !on) render_shop_leave(); s_shop = on; if (on) { s_ms = false; s_set = false; s_sel = -1; s_bat = false; } }
int  touch_port_take_shop(void) { int r = s_shop_act; s_shop_act = 0; return r; }
void touch_port_set_pill(bool up) { s_pill = up; }
bool touch_port_battery(void) { return s_bat; }
void touch_port_show_battery(bool on) { s_bat = on; if (on) { s_bat_us = esp_timer_get_time(); s_sel = -1; } }

#ifdef CONFIG_POCKET_TANK_WAVESHARE_349_V1
void touch_port_toggle_pages(void){
    if(s_cf||setup_active()||s_down||g_ui349.owner)return; /* Never cut a captured touch/confirmation in half. */
    if(!g_ui349.controls_hidden){
        s_hidden.valid=s_ms||s_set||s_shop||s_bat||s_sel>=0||g_ui349.slots_picker;g_ui349.controls_hidden=true;
        s_hidden.ms=s_ms;s_hidden.set=s_set;s_hidden.shop=s_shop;s_hidden.bat=s_bat;s_hidden.sel=s_sel;
        s_hidden.scroll=g_ui349.scroll;s_hidden.view=g_ui349.view;s_hidden.species=g_ui349.species_picker;
        s_hidden.slots=g_ui349.slots_picker;s_hidden.habitat=g_ui349.habitat_picker;s_hidden.species_page=g_ui349.species_page;
        s_ms=s_set=s_shop=s_bat=false;s_sel=-1;
        g_ui349.species_picker=g_ui349.slots_picker=g_ui349.habitat_picker=false;
        g_ui349.view=(ui349_view_t){0,"TANK",true,0};g_ui349.scroll=0;
    }else if(s_hidden.valid){
        g_ui349.controls_hidden=false;
        s_ms=s_hidden.ms;s_set=s_hidden.set;s_shop=s_hidden.shop;s_bat=s_hidden.bat;s_sel=s_hidden.sel;
        s_sel_us=s_bat_us=esp_timer_get_time();g_ui349.view=s_hidden.view;g_ui349.scroll=s_hidden.scroll;
        g_ui349.species_picker=s_hidden.species;g_ui349.slots_picker=s_hidden.slots;
        g_ui349.habitat_picker=s_hidden.habitat;g_ui349.species_page=s_hidden.species_page;s_hidden.valid=false;
    }else {g_ui349.controls_hidden=false;s_ms=true;}
    g_ui349.owner=0;s_back=false;s_held_page=false;
}
int touch_port_take_slot(void){int s=s_slot_request;s_slot_request=-1;return s;}
void touch_port_slot_changed(void){touch_port_dismiss();s_down=false;s_held_page=false;s_trimmed=false;s_cf=false;s_cf_ans=0;s_back=false;}
#endif
