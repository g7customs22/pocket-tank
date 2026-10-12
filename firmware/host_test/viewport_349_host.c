/* Validate real mapping code over every physical pixel, both orientations. */
#include <assert.h>
#include <stdio.h>
#include "viewport_349_v1.h"
int main(void) {
    int x, y;
    int left = V1_VIEW_X, right = left + V1_VIEW_W - 1;
    assert(v1_map_landscape(left, 0, false, &x, &y) && x == 0 && y == 0);
    assert(v1_map_landscape(right, 171, false, &x, &y) && x == TANK_W-1 && y == 367);
    assert(v1_map_landscape(639-right, 0, true, &x, &y) && x == TANK_W-1 && y == 367);
    assert(v1_map_landscape(639-left, 171, true, &x, &y) && x == 0 && y == 0);
    for (int inverted = 0; inverted < 2; inverted++) {
        int active = 0;
        for (int py = 0; py < 640; py++) for (int px = 0; px < 172; px++) {
            int tx, ty;
            bool display = v1_map_panel(px, py, inverted != 0, &x, &y);
            bool touch = v1_map_landscape(639 - py, px, inverted != 0, &tx, &ty);
            assert(display == touch);
            if (display) {
                assert(x >= 0 && x < TANK_W && y >= 0 && y < 368);
                assert(x == tx && y == ty);
                active++;
            }
        }
        assert(active == V1_VIEW_W * 172);
        assert(!v1_map_landscape(-1, 0, inverted != 0, &x, &y));
        assert(!v1_map_landscape(640, 0, inverted != 0, &x, &y));
        assert(!v1_map_landscape(320, -1, inverted != 0, &x, &y));
        assert(!v1_map_landscape(320, 172, inverted != 0, &x, &y));
        assert(!v1_map_landscape(0, 80, inverted != 0, &x, &y));
        assert(!v1_map_landscape(639, 80, inverted != 0, &x, &y));
    }
    puts("PASS: 220160 panel pixels, both rotations, bounds, corners, margins and touch alignment");
    return 0;
}
