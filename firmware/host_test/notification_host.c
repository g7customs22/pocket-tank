#include "notice.h"
#include "progression.h"
#include "render.h"
#include "ui_349_v1.h"
#include "sounds.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
bool persist_port_save(const void *p,size_t n){(void)p;(void)n;return true;}
bool persist_port_load(void *p,size_t n,size_t *got){(void)p;(void)n;*got=0;return false;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return 1800000000;}
const char *version_port_string(void){return "NOTICE_TEST";}
static void fresh(tank_t *t,int habitat){
 tank_init(t,23);progression_fresh(t);progression_setup_done(t);t->habitat=habitat;
 notice_sync(t);
}
static void pixels(tank_t *t,int kind,int fish,uint32_t bit){
 uint16_t *src=calloc(TANK_W*TANK_H,sizeof *src);
 uint16_t *buf=calloc(UI349_W*UI349_H+2,sizeof *buf);assert(src&&buf);
 buf[0]=0x1234;buf[UI349_W*UI349_H+1]=0x5678;
 render_notice(t,src,TANK_W,kind,fish,bit,1);
 for(int hidden=0;hidden<2;hidden++){
  ui349_state_t s={0};s.controls_hidden=hidden;
  ui349_configure(&s,(ui349_view_t){500,"NEWS",true,92});
  ui349_render(&s,t,src,buf+1);
  /* Every modal pixel, including title, caption and countdown, survives at 1:1. */
  unsigned lit=0;
  for(int y=100;y<256;y++)for(int x=56;x<392;x++){
   uint16_t v=src[y*TANK_W+x];lit+=v!=0;
   assert(buf[1+(y-s.scroll)*UI349_W+UI349_PAGE_X+x]==v);
  }
  assert(lit>1000&&buf[0]==0x1234&&buf[UI349_W*UI349_H+1]==0x5678);
  int x,y;assert(ui349_map(&s,300,100,&x,&y)&&y==192);
 }
 free(src);free(buf);
}
int main(void){
 /* Boundary checks use screen pixels; zoom and tilt must not change the band. */
 for(int zoom=0;zoom<2;zoom++)for(int edge=0;edge<3;edge++){
  ui349_state_t s={0};wp_camera_init(&s.world_camera);s.world_ready=true;
  s.world_camera.overview=!zoom;s.world_camera.y=edge==0?WP_Y_MIN:edge==1?WP_Y_CENTER:WP_Y_MAX;
  s.controls_hidden=true;ui349_configure(&s,(ui349_view_t){0,"TANK",true,0});
  for(int y=0;y<172;y++){
   int tx,ty;assert(ui349_map(&s,320,y,&tx,&ty));
   assert(ui349_feed_zone((float)y,false)==(y<30));
   assert(ui349_feed_zone((float)y,true)==(y<38));
  }
 }
 assert(!ui349_feed_zone(-1,false));assert(!ui349_feed_zone(172,true));
 puts("PASS: expanded feeding boundaries, both zoom modes and tilt extremes");
 tank_t t;
 for(int habitat=0;habitat<3;habitat++){
  fresh(&t,habitat);
  /* First feeding is credited when a player meal is eaten, not when food is dropped. */
  progression_tick(&t,0);notice_sync(&t);assert(!(t.tank_ms_bits&TMS_FIRST_FEEDING));
  t.player_feedings=1;progression_tick(&t,0);assert(t.tank_ms_bits&TMS_FIRST_FEEDING);
  notice_tick(&t,.1f,false);assert(notice_current()&&notice_current()->bit==TMS_FIRST_FEEDING);
  assert(notice_take_cue()==SND_MILESTONE);
  pixels(&t,NOTICE_TANK_MILESTONE,-1,TMS_FIRST_FEEDING);
  for(int i=0;i<59;i++)notice_tick(&t,.1f,false);
  assert(notice_current());notice_tick(&t,.2f,false);assert(!notice_current());
  /* Loading or switching into earned state must not replay old milestones. */
  notice_sync(&t);notice_tick(&t,1,false);assert(!notice_current()&&!notice_pending());
  t.fish[0].ms_bits|=MS_FIRST_MEAL_FROM_YOU;notice_tick(&t,.1f,true);
  assert(!notice_current()&&notice_pending()==1);
  for(int i=0;i<100;i++)notice_tick(&t,.1f,true);
  notice_tick(&t,.1f,false);assert(notice_current()&&notice_current()->bit==MS_FIRST_MEAL_FROM_YOU);
  assert(notice_take_cue()==SND_MILESTONE);
  notice_tick(&t,.1f,true);assert(!notice_current());notice_tick(&t,.1f,false);
  assert(notice_current()&&notice_current()->age==0&&notice_take_cue()==-1);
  pixels(&t,NOTICE_MILESTONE,0,MS_FIRST_MEAL_FROM_YOU);
  notice_dismiss();notice_tick(&t,1.1f,false);assert(!notice_current());
  notice_sync(&t);t.fish[0].stage=STAGE_ELDER;notice_tick(&t,.1f,false);
  assert(notice_current()&&notice_current()->kind==NOTICE_STAGE);
  pixels(&t,NOTICE_STAGE,0,0);
  notice_sync(&t);notice_low_battery();notice_low_battery();assert(notice_pending()==1);
  notice_tick(&t,.1f,false);assert(notice_current()->kind==NOTICE_LOW_BATTERY);
  pixels(&t,NOTICE_LOW_BATTERY,-1,0);
  /* A slot adoption clears stale fish indices and events from the previous tank. */
  fresh(&t,(habitat+1)%3);notice_tick(&t,.1f,false);assert(!notice_current()&&!notice_pending());
 }
 puts("PASS: all habitats; first feeding generation; fish/tank/stage/battery notices; six-second lifetime; blocked/resume/no-repeat cue; dismiss; slot sync; complete visible/hidden-control popup pixels and buffer guards");
 return 0;
}
