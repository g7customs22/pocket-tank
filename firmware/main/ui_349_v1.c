/* Native landscape shell. Font table copied from common/render.c so common
 * rendering and simulation stay unchanged. No ESP-IDF dependency: host-testable. */
#include "ui_349_v1.h"
#include "viewport_349_v1.h"
#include "pixels_349_v1.h"
#include "setup.h"
#include "progression.h"
#include "render.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "notice.h"

ui349_state_t g_ui349 = {.controls_hidden=true};
static const uint8_t FONT5X7[][7] = {
    { 0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 }, /* A */
    { 0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e }, /* B */
    { 0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e }, /* C */
    { 0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e }, /* D */
    { 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f }, /* E */
    { 0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10 }, /* F */
    { 0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f }, /* G */
    { 0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11 }, /* H */
    { 0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e }, /* I */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c }, /* J */
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 }, /* K */
    { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f }, /* L */
    { 0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11 }, /* M */
    { 0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 }, /* N */
    { 0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e }, /* O */
    { 0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10 }, /* P */
    { 0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d }, /* Q */
    { 0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11 }, /* R */
    { 0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e }, /* S */
    { 0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 }, /* T */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e }, /* U */
    { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04 }, /* V */
    { 0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a }, /* W */
    { 0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11 }, /* X */
    { 0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04 }, /* Y */
    { 0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f }, /* Z */
    { 0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e }, /* 0 */
    { 0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e }, /* 1 */
    { 0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f }, /* 2 */
    { 0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e }, /* 3 */
    { 0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02 }, /* 4 */
    { 0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e }, /* 5 */
    { 0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e }, /* 6 */
    { 0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 }, /* 7 */
    { 0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e }, /* 8 */
    { 0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c }, /* 9 */
    { 0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 }, /* ? */
    { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 }, /* ! */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c }, /* . */
    { 0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x08 }, /* , */
    { 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00 }, /* - */
    { 0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00 }, /* : */
    { 0x0c, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00 }, /* ' */
    { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 }, /* / */
    { 0x00, 0x04, 0x04, 0x1f, 0x04, 0x04, 0x00 }, /* + */
    { 0x19, 0x1a, 0x02, 0x04, 0x08, 0x0b, 0x13 }, /* % */
    { 0x00, 0x00, 0x08, 0x15, 0x02, 0x00, 0x00 }, /* ~ (the battery page's "about") */
};
static const char FONT_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789?!.,-:'/+%~";
static const uint8_t *glyph(char ch) {
    if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
    const char *p = ch ? strchr(FONT_CHARS, ch) : NULL;
    return p ? FONT5X7[p - FONT_CHARS] : NULL;
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static bool can_position(const ui349_state_t *s) {
#ifdef POCKET_TANK_WIDE_V1
    return s->view.key == 100 + SETUP_PG_BUBBLES || s->view.key == 100 + SETUP_PG_PLACE;
#else
    (void)s; return false;
#endif
}
/* Native-scale camera alongside the 124-pixel card, captured before overlays. */
#define FISH_VIEW_X 244
#define FISH_VIEW_W 300
static bool fish_view(const ui349_state_t *s) {
    return s->view.key >= 700 && s->view.key < 700 + N_FISH_MAX;
}
void ui349_capture_fish(ui349_state_t *s, const tank_t *t, const uint16_t *source, uint16_t *native) {
    if (!fish_view(s)) return;
    int i=s->view.key-700;
    if (i>=t->n_fish) return;
    s->camera_x=clamp((int)t->fish[i].x-FISH_VIEW_W/2,0,TANK_W-FISH_VIEW_W);
    s->camera_y=clamp((int)t->fish[i].y-UI349_H/2,0,TANK_H-UI349_H);
    for(int y=0;y<UI349_H;y++)
        memcpy(native+y*UI349_W+FISH_VIEW_X,
               source+(y+s->camera_y)*TANK_W+s->camera_x,FISH_VIEW_W*sizeof(uint16_t));
}
bool ui349_world_view(const ui349_state_t *s) { return !s->view.key || s->positioning; }
void ui349_configure(ui349_state_t *s, ui349_view_t view) {
    bool changed = s->view.key != view.key;
    if (changed) s->scroll = clamp(view.initial_scroll, 0, UI349_SCROLL_MAX);
    s->view = view;
    if (changed) { s->history_picker=false; s->positioning = can_position(s); s->species_picker=false; s->habitat_picker=false; }
}
bool ui349_map(const ui349_state_t *s, int x, int y, int *tx, int *ty) {
    if(s->history_picker || s->species_picker || s->habitat_picker || s->slots_picker
#ifdef POCKET_WORLD_PROTO
       ||s->copy_picker
#endif
    ) return false;
    if (x < 0 || x >= UI349_W || y < 0 || y >= UI349_H) return false;
#ifdef POCKET_WORLD_PROTO
    if(ui349_world_view(s)&&!s->positioning){
        if(!s->controls_hidden&&!s->copy_placing&&y>=6&&y<58)return false;
        float wx,wy;wp_map(&s->world_camera,s->copy_placing?s->copy_depth:1,(float)x,(float)y,&wx,&wy);*tx=(int)wx;*ty=(int)wy;return true;
    }
#endif
    if(ui349_clean_view(s)){*tx=x*(TANK_W-1)/(UI349_W-1);*ty=y*(TANK_H-1)/(UI349_H-1);return true;}
    /* Visible selector strip owns its gaps too: near-misses never tap/feed fish. */
    if(ui349_world_view(s)&&!s->positioning&&y>=6&&y<58)return false;
    if (ui349_world_view(s)) return v1_map_landscape(x, y, false, tx, ty);
    if (fish_view(s) && x >= FISH_VIEW_X && x < FISH_VIEW_X+FISH_VIEW_W) {
        *tx=s->camera_x+x-FISH_VIEW_X; *ty=s->camera_y+y; return true;
    }
    if (x < UI349_PAGE_X || x >= UI349_PAGE_X + TANK_UI_W) return false;
    *tx = x - UI349_PAGE_X; *ty = y + s->scroll;
    return true;
}
/* Hit targets match the rectangles drawn below; rails never feed tank gestures. */
static int rail_hit(const ui349_state_t *s, int x, int y) {
    if(s->history_picker)return x>=552&&x<632&&y>=12&&y<60?UI349_HISTORY_BACK:UI349_CONSUMED;
    if(s->view.key==400&&x>=8&&x<84&&y>=90&&y<110)return UI349_HISTORY;

#ifdef POCKET_WORLD_PROTO
    if(s->copy_picker){
        if(x>=552&&x<632&&y>=12&&y<60)return UI349_COPY_BACK;
        if(y>=60&&y<104){if(x>=104&&x<180)return UI349_COPY_PREV;if(x>=456&&x<532)return UI349_COPY_NEXT;if(x>=196&&x<440)return UI349_COPY_BUY;}
        if(s->copy_selected>=2&&y>=116&&y<160){if(x>=104&&x<236)return UI349_COPY_MOVE;if(x>=248&&x<380)return UI349_COPY_DEPTH;if(x>=392&&x<524)return UI349_COPY_SELL;}
        return UI349_CONSUMED;
    }
    if(s->copy_placing){if(x>=552&&x<632&&y>=8&&y<56)return UI349_COPY_DONE;return UI349_CONSUMED;}
    if(s->view.key==300&&x>=552&&x<632&&y>=66&&y<110)return UI349_COPIES;
#endif
    if(ui349_clean_view(s))return UI349_CONSUMED;
    if(s->slots_picker) {
        if(s->can_undo&&x>=552&&x<632&&y>=66&&y<110)return UI349_UNDO;
        if(x>=552&&x<632&&y>=12&&y<60)return UI349_TANKS_BACK;

        for(int i=0;i<3;i++)if(x>=104&&x<536&&y>=8+i*52&&y<52+i*52)return UI349_SLOT_PICK+i;
        return UI349_CONSUMED;
    }
    if(s->habitat_picker) {
        if(x>=552 && x<632 && y>=12 && y<60) return UI349_HABITAT_BACK;
        for(int i=0;i<HABITAT_COUNT;i++)
            if(x>=104 && x<536 && y>=8+i*52 && y<52+i*52) return UI349_HABITAT_PICK+i;
        return UI349_CONSUMED;
    }
    if(!s->view.key && !s->positioning && y>=8 && y<56)
        for(int i=0;i<3;i++)if(x>=8+i*176 && x<176+i*176)return UI349_SLOT_PICK+i;
    if(s->species_picker) {
        if(x>=552 && x<632 && y>=12 && y<60) return UI349_SPECIES_BACK;
        if(x>=552&&x<632&&y>=68&&y<108)return s->species_page>0?UI349_SPECIES_PREV:UI349_CONSUMED;
        if(x>=552&&x<632&&y>=116&&y<160){int n=0;for(int i=0;i<SPECIES_COUNT;i++)n+=tank_species_allowed(s->habitat,i);return (s->species_page+1)*6<n?UI349_SPECIES_NEXT:UI349_CONSUMED;}
        for(int n=0,i=0;i<SPECIES_COUNT;i++) {
            if(!tank_species_allowed(s->habitat,i))continue;
            int cell=n++-s->species_page*6;if(cell<0||cell>=6)continue;
            int bx=104+(cell%2)*224, by=8+(cell/2)*52;
            if(x>=bx && x<bx+212 && y>=by && y<by+44) return tank_species_allowed(s->habitat,i)?UI349_SPECIES_PICK+i:UI349_CONSUMED;
        }
        return UI349_CONSUMED;
    }
    if (s->positioning) {
        if (x >= 8 && x < 118 && y >= 8 && y < 44) return 12;
        if (x >= 552 && x < 632 && y >= 8 && y < 44) return UI349_NEXT;
    } else if (s->view.key) {
#ifdef POCKET_WORLD_PROTO
        if(s->view.key==200&&x>=552&&x<632){
            if(y>=66&&y<110)return UI349_CENTER_VIEW;
            if(y>=116&&y<160)return UI349_ZOOM;
        }
#endif
        if(s->view.key==400 && x>=8 && x<84 && y>=6 && y<34)return UI349_TANKS;
        if (x >= 8 && x < 84) {
            if (y >= 40 && y < 88 && s->scroll > 0) return 10;
            if (y >= 112 && y < 160 && s->scroll < UI349_SCROLL_MAX) return 11;
        }
        if (s->view.can_close && x >= 552 && x < 632 && y >= 12 && y < 60) return UI349_CLOSE;
        if (s->view.can_close && x >= 552 && x < 632) {
            if (y >= 66 && y < 110) return fish_view(s)?UI349_SPECIES:UI349_SETTINGS;
            if (y >= 116 && y < 160) return UI349_SHOP;
        }
        if (can_position(s) && x >= 552 && x < 632 && y >= 12 && y < 60) return 12;
#ifdef POCKET_TANK_WIDE_V1
    } else if (x >= 552 && x < 632 && y >= 8 && y < 56) {
        return UI349_MENU;
#else
    } else if (x >= 440 && x < 624) {
        if (y >= 12 && y < 54) return UI349_MENU;
        if (y >= 65 && y < 107) return UI349_SETTINGS;
        if (y >= 118 && y < 160) return UI349_SHOP;
#endif
    }
    return UI349_CONSUMED;
}
int ui349_input(ui349_state_t *s, bool down, int x, int y, bool *content_down, int *tx, int *ty) {
    *content_down = false;
#ifdef POCKET_WORLD_PROTO
    s->world_camera.frozen=down;
#endif
    if (down && !s->owner) {
        s->owner = rail_hit(s,x,y) != UI349_CONSUMED ? 2 : ui349_map(s, x, y, tx, ty) ? 1 : 2;
        s->pressed = rail_hit(s, x, y); s->press_key = s->view.key;
        s->press_x=x; s->press_y=y; s->press_scroll=s->scroll; s->rail_moved=false;
    }
    /* A menu drag owns its whole contact, including release. The adapter
     * cancels its pending tap as soon as this owner is acquired. */
    bool scrollable=s->view.key==300 || s->view.key==400 || s->view.key>=2000;
    if(s->owner==1 && down && scrollable && abs(y-s->press_y)>12)s->owner=3;
    if(s->owner==3){
        if(down)s->scroll=clamp(s->press_scroll+s->press_y-y,0,UI349_SCROLL_MAX);
        else s->owner=0;
        return UI349_CONSUMED;
    }
    if (s->owner == 2) {
        if (down) {
            int dx=x-s->press_x, dy=y-s->press_y;
            if (dx*dx+dy*dy>16*16) s->rail_moved=true;
            s->last_x = x; s->last_y = y; return UI349_CONSUMED;
        }
        int action = !s->rail_moved && s->press_key == s->view.key && s->pressed == rail_hit(s, s->last_x, s->last_y)
                     ? s->pressed : UI349_CONSUMED;
        s->owner = 0;
        if (action == 10) { s->scroll = clamp(s->scroll - UI349_SCROLL_STEP, 0, UI349_SCROLL_MAX); return UI349_CONSUMED; }
        if (action == 11) { s->scroll = clamp(s->scroll + UI349_SCROLL_STEP, 0, UI349_SCROLL_MAX); return UI349_CONSUMED; }
        if (action == 12) { s->positioning = !s->positioning; return UI349_CONSUMED; }
        return action;
    }
    if (down) {
        /* Keep a captured drag in the content; crossing a rail cannot tap it. */
        int left = ui349_clean_view(s)?0:ui349_world_view(s) ? V1_VIEW_X : UI349_PAGE_X;
        int width = ui349_clean_view(s)?UI349_W:ui349_world_view(s) ? V1_VIEW_W : TANK_UI_W;
#ifdef POCKET_WORLD_PROTO
        if(ui349_world_view(s)&&!s->positioning){left=0;width=UI349_W;}
#endif
        (void)ui349_map(s, clamp(x, left, left + width - 1), clamp(y, 0, UI349_H - 1), tx, ty);
        s->last_x = *tx; s->last_y = *ty; *content_down = true;
    } else { *tx = s->last_x; *ty = s->last_y; s->owner = 0; }
    return UI349_PASS;
}
static uint16_t color(unsigned rgb) {
    return (uint16_t)(((rgb >> 8) & 0xf800) | ((rgb >> 5) & 0x7e0) | ((rgb >> 3) & 31));
}
static void rect(uint16_t *fb, int x, int y, int w, int h, unsigned rgb) {
    uint16_t c = color(rgb);
    for (int py = clamp(y, 0, UI349_H); py < clamp(y+h, 0, UI349_H); py++)
        for (int px = clamp(x, 0, UI349_W); px < clamp(x+w, 0, UI349_W); px++) fb[py*UI349_W+px] = c;
}
static void text(uint16_t *fb, int x, int y, int scale, unsigned rgb, const char *str) {
    for (; *str; str++, x += 6*scale) {
        const uint8_t *g = glyph(*str);
        if (!g) continue;
        for (int r=0; r<7; r++) for (int c=0; c<5; c++)
            if (g[r] & (16 >> c)) rect(fb,x+c*scale,y+r*scale,scale,scale,rgb);
    }
}
static void button(uint16_t *fb, int x, int y, int w, int h, const char *label, int scale, bool enabled) {
    rect(fb,x,y,w,h,enabled ? 0x265b62 : 0x122a33);
    int tw = (int)strlen(label)*6*scale-scale;
    text(fb,x+(w-tw)/2,y+(h-7*scale)/2,scale,enabled ? 0xf2ffff : 0x66838a,label);
}
static void ui349_render_body(const ui349_state_t *s, const tank_t *t, const uint16_t *source, uint16_t *native) {
    if(s->history_picker){
        rect(native,0,0,UI349_W,UI349_H,0x0b2029);
        text(native,104,8,2,0xf2ffff,"RECENT EVENTS");
        text(native,104,29,1,0xa9e2dd,"THIS SESSION / NEWEST FIRST");
        for(int i=0;i<notice_history_count();i++){
            const notice_history_t *h=notice_history_get(i);char row[72];
            snprintf(row,sizeof row,"%s / %.48s",HABITAT_NAMES[h->habitat<HABITAT_COUNT?h->habitat:0],h->title);
            text(native,104,47+i*17,1,0xa9e2dd,row);
        }
        if(!notice_history_count())text(native,104,65,1,0xa9e2dd,"NEW EVENTS WILL APPEAR HERE");
        button(native,552,12,80,48,"BACK",2,true);return;
    }

#ifdef POCKET_WORLD_PROTO
    if(s->copy_picker){
        rect(native,0,0,UI349_W,UI349_H,0x0b2029);char label[70];int item=s->copy_item;
        text(native,104,10,2,0xe4dfb8,item==WP_AIRSTONE?"AIR STONE":SD_ITEMS[item].name);
        if(s->copy_error)snprintf(label,sizeof label,"NOT ENOUGH CREDIT OR SPACE");else snprintf(label,sizeof label,"%d CREDITS   %d PLACED",(int)t->sd_balance,wp_quantity(t,&t->world,item));text(native,104,35,1,0xa9e2dd,label);
        button(native,552,12,80,48,"BACK",2,true);button(native,104,60,76,44,"PREV",2,true);button(native,456,60,76,44,"NEXT",2,true);
        snprintf(label,sizeof label,"ADD / %d",item==WP_AIRSTONE?40:SD_ITEMS[item].price);button(native,196,60,244,44,label,2,true);
        if(s->copy_selected>=2){button(native,104,116,132,44,"MOVE",2,true);button(native,248,116,132,44,s->copy_depth==0?"BACK":s->copy_depth==1?"MIDDLE":"FRONT",2,true);button(native,392,116,132,44,s->copy_sell_armed?"CONFIRM?":"SELL",2,true);}
        else text(native,104,128,1,0xa9e2dd,s->copy_error?"NOT ENOUGH CREDIT OR SPACE":"HOLD A PLACED COPY TO EDIT IT");
        return;
    }
#endif

    if(s->slots_picker) {
        rect(native,0,0,UI349_W,UI349_H,0x0b2029);
        text(native,6,16,1,0xa9e2dd,"TANKS");
        for(int i=0;i<3;i++) {
            char label[32];snprintf(label,sizeof label,"%s  %d FISH  %s",HABITAT_NAMES[i],s->active_slot==(unsigned)i?t->n_fish:s->slot_fish[i],s->active_slot==(unsigned)i?"ACTIVE":(s->slots_present&(1u<<i))?"PAUSED":"NEW");
            if(s->active_slot==(unsigned)i)rect(native,102,6+i*52,436,48,0xd8bd76);
            button(native,104,8+i*52,432,44,label,2,true);
        }
        button(native,552,12,80,48,"BACK",2,true);

        if(s->can_undo){button(native,552,66,80,44,"UNDO",2,true);text(native,552,115,1,0xa9e2dd,"LAST RESET");}
        text(native,552,130,1,0xa9e2dd,s->slot_error?"SAVE ERROR":"OTHERS PAUSE");
        return;
    }
    if(s->habitat_picker) {
        rect(native,0,0,UI349_W,UI349_H,0x0b2029);
        text(native,6,16,1,0xa9e2dd,"HABITAT");
        for(int i=0;i<HABITAT_COUNT;i++) {
            int y=8+i*52;
            if(t->habitat==i) rect(native,102,y-2,436,48,0xd8bd76);
            button(native,104,y,432,44,HABITAT_NAMES[i],2,true);
        }
        button(native,552,12,80,48,"BACK",2,true);
        text(native,552,82,1,0xa9e2dd,"KEEPS FISH");
        text(native,552,98,1,0xa9e2dd,"AND ITEMS");
        text(native,552,130,1,0xa9e2dd,"FREE CHANGE");
        return;
    }
    if(s->species_picker && fish_view(s)) {
        rect(native,0,0,UI349_W,UI349_H,0x0b2029);
        text(native,6,16,1,0xa9e2dd,"SPECIES");
        text(native,6,36,1,0xa9e2dd,HABITAT_NAMES[t->habitat]);
        int fish=s->view.key-700;
        for(int n=0,i=0;i<SPECIES_COUNT;i++) {
            if(!tank_species_allowed(t->habitat,i))continue;
            int cell=n++-s->species_page*6;if(cell<0||cell>=6)continue;
            int x=104+(cell%2)*224,y=8+(cell/2)*52;
            if(fish<t->n_fish && t->fish_species[fish]==i) rect(native,x-2,y-2,216,48,0xd8bd76);
            button(native,x,y,212,44,TANK_SPECIES[i].name,2,tank_species_allowed(t->habitat,i));
        }
        button(native,552,12,80,48,"BACK",2,true);
        button(native,552,68,80,40,"PREV",2,s->species_page>0);
        int available=0;for(int i=0;i<SPECIES_COUNT;i++)available+=tank_species_allowed(t->habitat,i);
        button(native,552,116,80,44,"NEXT",2,(s->species_page+1)*6<available);
        return;
    }
    if (fish_view(s)) {
        rect(native,0,0,FISH_VIEW_X,UI349_H,0x0b2029);
        rect(native,FISH_VIEW_X+FISH_VIEW_W,0,UI349_W-FISH_VIEW_X-FISH_VIEW_W,UI349_H,0x0b2029);
    } else rect(native,0,0,UI349_W,UI349_H,0x0b2029);
    if (!ui349_world_view(s)) {
        for (int y=0; y<UI349_H; y++)
            memcpy(native+y*UI349_W+UI349_PAGE_X,source+(y+s->scroll)*TANK_W,(fish_view(s) ? FISH_VIEW_X-UI349_PAGE_X : TANK_UI_W)*sizeof(uint16_t));
        if(s->view.key==400){button(native,8,6,76,28,"TANKS",2,true);button(native,8,90,76,20,"HISTORY",1,true);}
        else text(native,6,10,2,0xa9e2dd,s->view.title);
        button(native,8,40,76,48,"UP",3,s->scroll>0);
        button(native,8,112,76,48,"DOWN",3,s->scroll<UI349_SCROLL_MAX);
        rect(native,89,0,3,UI349_H,0x265b62);
        rect(native,89,s->scroll*UI349_H/TANK_H,3,UI349_H*UI349_H/TANK_H,0xa9e2dd);
        if (s->view.can_close) button(native,552,12,80,48,"TANK",2,true);
        if (can_position(s)) button(native,552,12,80,48,"MOVE",2,true);
        if (s->view.can_close) {
            button(native,552,66,80,44,fish_view(s)?"SPECIES":s->view.key==300?"COPIES":"SETTINGS",1,true);
            button(native,552,116,80,44,"SHOP",2,true);
#ifdef POCKET_WORLD_PROTO
            if(s->view.key==200){
                button(native,552,66,80,44,"",1,true);
                text(native,574,71,1,0xa9e2dd,"CENTER");text(native,580,82,1,0xa9e2dd,"VIEW");
                text(native,556,99,1,0xa9e2dd,s->center_result==1?"CENTERED":s->center_result<0?"TRY AGAIN":"HOLD STILL");
                button(native,552,116,80,44,"",1,true);
                text(native,562,123,1,0xa9e2dd,s->world_camera.overview?"ZOOM OFF":"ZOOM ON");
                text(native,562,140,1,0xa9e2dd,s->world_camera.overview?"FULL TANK":"TILT VIEW");
            }
#endif
        } else {
        text(native,552,86,2,0xa9e2dd,s->scroll == 0 ? "TOP" : s->scroll == UI349_SCROLL_MAX ? "END" : "MIDDLE");
        text(native,552,120,2,0xa9e2dd,"USE UP");
        text(native,552,142,2,0xa9e2dd,"OR DOWN");
        }
        if(fish_view(s) && s->view.key-700<t->n_fish) {
            int species=t->fish_species[s->view.key-700];
            text(native,252,8,1,0xf2ffff,TANK_SPECIES[species<SPECIES_COUNT?species:0].name);
            text(native,252,20,1,0xa9e2dd,tank_species_role(species));
        }
    } else {
#ifdef POCKET_WORLD_PROTO
        if(!s->positioning){
            render_world_proto(t,&t->world,&s->world_camera,native);
            if(s->copy_placing){button(native,552,8,80,48,"DONE",2,true);text(native,8,12,1,0xe4dfb8,"DRAG TO PLACE");return;}
            if(s->controls_hidden)return;
            for(int i=0;i<3;i++){if((unsigned)i==s->active_slot)rect(native,6+i*176,6,172,52,0xd8bd76);button(native,8+i*176,8,168,48,i==0?"STANDARD":i==1?"FRESHWATER":"SALTWATER",2,true);}
            button(native,552,8,80,48,"MENU",2,true);
            char level[12];if(s->battery.mv)snprintf(level,sizeof level,"~%d%%",s->battery.pct);else snprintf(level,sizeof level,"--%%");
            text(native,8,158,1,s->battery.low?0xffce73:0xf2ffff,level);
            if(s->battery.warning_s)text(native,55,158,1,0xffce73,"LOW BATTERY");
            text(native,440,158,1,0xa9e2dd,t->uv.style?(t->night?"UV / OFF":"UV / ON"):(t->night?"WHITE / OFF":"WHITE / ON"));
            if(s->feed_notice_s)text(native,248,148,1,0xffce73,"FOOD LIMIT - WAIT FOR FISH");
            if(s->slot_error)text(native,248,128,1,0xffce73,"SAVE UNAVAILABLE");
            render_sd_toast_native(t,native,UI349_W);return;
        }
#endif
        if(ui349_clean_view(s)){
            for(int y=0;y<UI349_H;y++)for(int x=0;x<UI349_W;x++)
                native[y*UI349_W+x]=source[(y*(TANK_H-1)/(UI349_H-1))*TANK_W+x*(TANK_W-1)/(UI349_W-1)];
            return;
        }
        v1_compose_world(source,native);
#ifdef POCKET_TANK_WIDE_V1
        (void)t;
        if (s->positioning) {
            button(native,8,8,110,36,"OPTIONS",2,true);
            text(native,200,14,2,0xf2ffff,"DRAG TO POSITION");
            button(native,552,8,80,36,"DONE",2,true);
        } else {
            for(int i=0;i<3;i++){
                int x=8+i*176;
                if(s->active_slot==(unsigned)i)rect(native,x-2,6,172,52,0xd8bd76);
                button(native,x,8,168,48,HABITAT_NAMES[i],2,true);
                if(s->active_slot==(unsigned)i)rect(native,x+12,50,144,3,0xd8bd76);
            }
            button(native,552,8,80,48,"MENU",2,true);
            /* Only glyph outlines touch the scene: no rectangle or panel. */
            char level[12];
            if(s->battery.mv)snprintf(level,sizeof level,"~%d%%",s->battery.pct);
            else snprintf(level,sizeof level,"--%%");
            for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)
                if(dx||dy)text(native,8+dx,158+dy,1,0x061014,level);
            text(native,8,158,1,s->battery.low?0xffce73:0xf2ffff,level);
            if(s->battery.warning_s)text(native,55,158,1,0xffce73,"LOW BATTERY");
            text(native,440,158,1,0xa9e2dd,t->uv.style?(t->night?"UV / OFF":"UV / ON"):(t->night?"WHITE / OFF":"WHITE / ON"));
            if(s->feed_notice_s)text(native,248,148,1,0xffce73,"FOOD LIMIT - WAIT FOR FISH");
            if(s->slot_error)text(native,248,128,1,0xffce73,"SAVE UNAVAILABLE");
        }
#else
        text(native,10,10,3,0xf2ffff,"POCKET TANK");
        for (int i=0; i<t->n_fish && i<N_FISH_MAX; i++) {
            int y=43+i*21;
            rect(native,12,y,8,14,t->fish[i].color);
            text(native,30,y,2,0xa9e2dd,t->fish[i].name);
        }
        button(native,440,12,184,42,"MENU",3,true);
        button(native,440,65,184,42,"SETTINGS",3,true);
        button(native,440,118,184,42,"SHOP",3,true);
#endif
    }
}

void ui349_message(const char *message){snprintf(g_ui349.feedback,sizeof g_ui349.feedback,"%s",message);g_ui349.feedback_s=3;}
void ui349_render(const ui349_state_t *s,const tank_t *t,const uint16_t *source,uint16_t *native){
    ui349_render_body(s,t,source,native);
    if(s->feedback_s>0){rect(native,168,148,304,22,0x04141a);text(native,180,155,1,0xf2ffff,s->feedback);}
}
