/* Native portrait pixels -> logical swimming world. No ESP-IDF dependencies.
 * Landscape is 640x172; fit world aspect ratio without horizontal stretching.
 * Raw AXS touch is landscape in the V1 example (X long axis, Y short axis).
 */
#ifndef VIEWPORT_349_V1_H
#define VIEWPORT_349_V1_H
#include <stdbool.h>
#include "tank.h"
/* Feedback-driven V1 backlight: keep nonzero requests above the observed
 * cutoff. 153 is the lowest drive previously confirmed visible on this board.
 * Percentages remain relative user levels, not calibrated luminance. */
static inline unsigned v1_backlight_drive(unsigned level) {
    return level ? 153u + (level - 1u) * 102u / 254u : 0u;
}
/* Additional contrast between user levels without entering the LED cutoff.
 * Q8 gains are a visual starting point, not measured luminance percentages. */
static inline unsigned v1_image_gain(unsigned percent) {
    return percent <= 30 ? 128u : percent <= 60 ? 192u : 256u;
}
static inline uint16_t v1_dim_pixel(uint16_t p, unsigned gain) {
    unsigned r=(((p>>11)&31)*gain+128)>>8;
    unsigned g=(((p>>5)&63)*gain+128)>>8;
    unsigned b=((p&31)*gain+128)>>8;
    return (uint16_t)((r<<11)|(g<<5)|b);
}
#define V1_PANEL_W 172
#define V1_PANEL_H 640
#define V1_VIEW_W (TANK_W * V1_PANEL_W / TANK_H)
#define V1_VIEW_X ((V1_PANEL_H - V1_VIEW_W) / 2)
_Static_assert((TANK_W == 448 || TANK_W == 1344) && TANK_H == 368, "V1 world dimensions need review");

static inline bool v1_map_landscape(int x, int y, bool inverted, int *tx, int *ty) {
    if (x < 0 || x >= V1_PANEL_H || y < 0 || y >= V1_PANEL_W) return false;
    if (inverted) { x = V1_PANEL_H - 1 - x; y = V1_PANEL_W - 1 - y; }
    if (x < V1_VIEW_X || x >= V1_VIEW_X + V1_VIEW_W) return false;
    *tx = (x - V1_VIEW_X) * (TANK_W - 1) / (V1_VIEW_W - 1);
    *ty = y * (TANK_H - 1) / (V1_PANEL_W - 1);
    return true;
}
static inline bool v1_map_panel(int px, int py, bool inverted, int *tx, int *ty) {
    return v1_map_landscape(V1_PANEL_H - 1 - py, px, inverted, tx, ty);
}
#endif
