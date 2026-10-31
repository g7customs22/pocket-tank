#define main previous_clock_ui_main
#include "clock_ui_host.c"
#undef main

int main(void) {
    const float starts[] = {1080144.f, 1048576.f, 32767.99f, 200000.f};
    volatile float old_clock = starts[0];
    old_clock += GAME_CLOCK_STEP;
    assert(old_clock == starts[0]); /* Reproduce the installed float stall. */
    uint16_t *a = calloc(WP_W * WP_H, sizeof *a);
    uint16_t *b = calloc(WP_W * WP_H, sizeof *b);
    assert(a && b);
    for (int habitat = 0; habitat < HABITAT_COUNT; habitat++) {
        for (unsigned sample = 0; sample < sizeof starts / sizeof *starts; sample++) {
            tank_t t; healthy(&t); t.habitat = habitat; t.clock = starts[sample];
            float age = progression_age_s(&t, 0);
            int money = t.sd_balance; uint32_t milestones = t.fish[0].ms_bits;
            for (int z = 0; z < 6; z++) t.fish[0].zone_seen[z] = t.clock - 10 - z;
            /* Zero elapsed sleep does nothing, including on an old snapshot. */
            tank_t saved = t; tank_tick_sleep(&t, 0); assert(!memcmp(&saved, &t, sizeof t));
            tank_tick_sleep(&t, 1);
            assert(t.clock < 32768 && t.clock >= 0);
            for (int z = 0; z < 6; z++)
                assert(fabsf((t.clock - t.fish[0].zone_seen[z]) - (11 + z)) < .02f);
            assert(progression_age_s(&t, 0) == age && t.sd_balance == money && t.fish[0].ms_bits == milestones);

            /* Independent fish-free world and ornament-only pixel comparisons. */
            t.clock = starts[sample]; t.n_fish = 0; t.sd_unlocks = 0;
            memset(t.food, 0, sizeof t.food); memset(t.algae, 0, sizeof t.algae);
            wp_camera_t camera; wp_camera_init(&camera);
            tank_tick(&t, GAME_CLOCK_STEP, advisor_rules);
            assert(t.clock < 32768);
            float before = t.clock;
            render_world_proto(&t, &t.world, &camera, a);
            for (int tick = 0; tick < 30; tick++) tank_tick(&t, GAME_CLOCK_STEP, advisor_rules);
            assert(t.clock > before && fabsf(t.clock - before - 1) < .02f);
            render_world_proto(&t, &t.world, &camera, b);
            assert(memcmp(a, b, WP_W * WP_H * sizeof *a));
            memset(a, 0, WP_W * WP_H * sizeof *a); memset(b, 0, WP_W * WP_H * sizeof *b);
            ctx_t ca = {a, WP_W, 1, 0, 0, WP_W, WP_H};
            ctx_t cb = {b, WP_W, 1, 0, 0, WP_W, WP_H};
            draw_extra(&ca, 6, 200, 130, before, 1, false);
            draw_extra(&cb, 6, 200, 130, t.clock, 1, false);
            assert(memcmp(a, b, WP_W * WP_H * sizeof *a));
        }
    }
    free(a); free(b);
    puts("PASS old-clock stall reproduced; all habitats advance and animate; rollover/sleep visit ages, money, milestones and progression age preserved");
    return 0;
}
