#include "ui_349_v1.h"
#include "render.h"
#include "setup.h"
#include "progression.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
bool persist_port_load(void *b,size_t n,size_t *got){(void)b;(void)n;*got=0;return false;}
bool persist_port_save(const void *b,size_t n){(void)b;(void)n;return true;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return 0;}
const char *version_port_string(void){return "PREVIEW";}
int main(void) {
 tank_t t;tank_init(&t,23);progression_boot(&t);
 size_t n=TANK_W*TANK_H;
 uint16_t *frame=calloc(n+2,2),*cache=calloc(n+2,2);
 uint32_t *dirty=calloc(RENDER_DIRTY_WORDS+2,4);
 assert(frame && cache && dirty);
 frame[0]=cache[0]=0x1357; frame[n+1]=cache[n+1]=0x2468;
 dirty[0]=0x13572468;dirty[RENDER_DIRTY_WORDS+1]=0x24681357;
 render_set_scene_cache(cache+1);render_set_dirty_mask(dirty+1);
 for(int habitat=0;habitat<HABITAT_COUNT;habitat++)
 for(int species=0;species<SPECIES_COUNT;species++) for(int edge=0;edge<4;edge++) {
  t.habitat=habitat;
  assert(tank_set_species(&t,0,species));t.fish[0].size=1.6f;
  t.fish[0].x=(edge&1)?TANK_W-1:0;t.fish[0].y=(edge&2)?TANK_H-1:0;
  t.fish[0].yaw=t.fish[0].yaw_tail=(edge&1)?-1:1;t.fish[0].heading=edge*1.57f;
  t.fish[0].glass_attached=species==SPECIES_PLECO;
  render_tank(&t,frame+1,TANK_W);
  assert(frame[0]==0x1357 && frame[n+1]==0x2468);
  assert(cache[0]==0x1357 && cache[n+1]==0x2468);
  assert(dirty[0]==0x13572468 && dirty[RENDER_DIRTY_WORDS+1]==0x24681357);
 }
 puts("PASS all species cached rendering at four edges, frame/cache/dirty-mask guards");
 render_set_scene_cache(NULL);render_set_dirty_mask(NULL);free(frame);free(cache);free(dirty);
 return 0;
}
