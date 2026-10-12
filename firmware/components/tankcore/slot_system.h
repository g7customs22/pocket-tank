#ifndef SLOT_SYSTEM_H
#define SLOT_SYSTEM_H
#include "tank.h"
/* 0 = legacy/migration needed, 1 = selected slot loaded, -1 = blocked. */
int slot_system_boot(tank_t *tank);
bool slot_system_migrate(tank_t *tank);
bool slot_system_enabled(void);
bool slot_system_blocked(void);
bool slot_system_save(void);
bool slot_system_switch(tank_t *tank,unsigned slot);
bool slot_system_reset(tank_t *tank,uint32_t seed);
unsigned slot_system_active(void);
unsigned slot_system_present(void);
bool slot_system_undo(tank_t *tank);
bool slot_system_has_undo(void);
int slot_system_fish_count(unsigned slot);
#endif
