#ifndef WORLD_TYPES_H
#define WORLD_TYPES_H
#include <stdint.h>
#define WP_COPIES 8
#define WP_SOURCES 4
#define WP_AIRSTONE 25
typedef struct { uint16_t id; uint8_t item,depth; float x; } wp_copy_t;
typedef struct { wp_copy_t copy[WP_COPIES]; } wp_saved_t;
typedef struct { wp_saved_t saved; uint16_t target[6]; uint8_t assigned; } wp_objects_t;
#endif
