/* Host prototype contract. Not part of the installed v5 tank/save layout. */
#ifndef WORLD_PROTO_H
#define WORLD_PROTO_H
#include "tank.h"
#define WP_W 640
#define WP_H 172
typedef struct { float x,y,neutral_roll,neutral_pitch; bool ready,frozen; int64_t last_sample; unsigned rejected; } wp_camera_t;
typedef struct { uint16_t id; float x,y; } wp_source_t;
void wp_camera_init(wp_camera_t *c);
bool wp_camera_step(wp_camera_t *c,const int16_t a[3],bool inverted,int64_t sample_us,int64_t now_us,float dt);
void wp_recenter(wp_camera_t *c,const int16_t a[3],bool inverted);
void wp_origin(const wp_camera_t *c,int depth,float *x,float *y);
void wp_map(const wp_camera_t *c,int depth,float sx,float sy,float *x,float *y);
void wp_project(const wp_camera_t *c,int depth,float x,float y,float *sx,float *sy);
bool wp_valid(const wp_saved_t *saved);
int wp_quantity(const tank_t *t,const wp_objects_t *o,int item);
int wp_buy(tank_t *t,wp_objects_t *o,int item,float x,int depth);
bool wp_move(wp_objects_t *o,int id,float x,int depth);
bool wp_sell(tank_t *t,wp_objects_t *o,int id);
int wp_sources(const tank_t *t,const wp_objects_t *o,wp_source_t out[WP_SOURCES]);
void wp_bind(tank_t *t,wp_objects_t *o);
void wp_bubble(tank_t *t,int fish,float *x,float *y,bool remember);
void wp_tick(tank_t *t,wp_objects_t *o,float dt,advisor_fn advise);
int wp_hit(const wp_camera_t *c,const wp_objects_t *o,float sx,float sy,int previous);
void wp_wipe(tank_t *t,const wp_camera_t *c,float sx0,float sy0,float sx1,float sy1);
void tank_proto_wipe(tank_t *t,float x0,float y0,float x1,float y1);
void render_world_proto(const tank_t *t,const wp_objects_t *o,const wp_camera_t *camera,uint16_t *fb);
#endif
