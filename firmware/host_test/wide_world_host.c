#include "tank.h"
#include "progression.h"
#include "render.h"
#include "setup.h"
#include "advisor_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
static unsigned char saved[4096];static size_t saved_n;
bool persist_port_load(void *b,size_t n,size_t *got){if(!saved_n)return false;*got=saved_n<n?saved_n:n;memcpy(b,saved,*got);return true;}
bool persist_port_save(const void *b,size_t n){assert(n<=sizeof saved);memcpy(saved,b,n);saved_n=n;return true;}
bool persist_port_erase(void){saved_n=0;return true;}
int64_t clock_port_now_unix(void){return 0;}
const char *version_port_string(void){return "HOST";}
static void fixture(const char *name){
 char path[256];snprintf(path,sizeof path,"work/pocket-tank/sim/testdata/saves/%s",name);
 FILE *f=fopen(path,"rb");assert(f);saved_n=fread(saved,1,sizeof saved,f);fclose(f);
 tank_t a,b;tank_init(&a,11);progression_boot(&a);assert(a.n_fish>=2);
 assert(progression_save(&a));assert(saved_n==1704);
 tank_init(&b,12);progression_boot(&b);assert(a.n_fish==b.n_fish);
 for(int i=0;i<a.n_fish;i++){
  assert(!strcmp(a.fish[i].name,b.fish[i].name));assert(a.fish[i].color==b.fish[i].color);
  assert(a.fish[i].stage==b.fish[i].stage);assert(fabsf(a.fish[i].hunger-b.fish[i].hunger)<.001f);
 }
 printf("PASS save round-trip: %s\n",name);
}
int main(void){ setbuf(stdout,NULL);
 assert(TANK_W==1344 && TANK_H==368 && ALGAE_CELLS==644);
 const char *files[]={"2026-09-29-1672-v0.2.0.sav","2026-09-29-1664-shrimp.sav","2026-09-29-1656.sav","2026-09-24-1656-sim.sav","2026-09-13-1432-prebubble.sav","2026-09-04-1304-device.sav"};
 for(int i=0;i<6;i++)fixture(files[i]);
 tank_t t;tank_init(&t,9);tank_new_population(&t);assert(t.n_fish>=2);
 memset(t.food,0,sizeof t.food); tank_feed(&t,1200,3);int pellets=0;
 for(int i=0;i<MAX_FOOD;i++)if(t.food[i].alive){assert(t.food[i].x>1100 && t.food[i].x<TANK_W);pellets++;}assert(pellets==3);
 tank_set_bubble_x(&t,1200);assert(t.bubble_x>1100);
 char encoded[512];
 for(int row=0;row<2;row++)for(int col=0;col<3;col++){
  t.fish[0].x=(col+.5f)*TANK_W/3;t.fish[0].y=(row+.5f)*TANK_H/2;
  advisor_core_encode(&t,0,encoded,sizeof encoded);char zone[16];snprintf(zone,sizeof zone,"zone %d ",row*3+col+1);assert(strstr(encoded,zone));
 }
 t.fish[0].x=1200;t.fish[0].y=150;t.fish[0].heading=0;
 memset(t.food,0,sizeof t.food);t.food[0].alive=true;t.food[0].x=1220;t.food[0].y=150;
 advisor_core_encode(&t,0,encoded,sizeof encoded);assert(strstr(encoded,"food near 12"));
 t.food[0].x=800;advisor_core_encode(&t,0,encoded,sizeof encoded);assert(strstr(encoded,"food none"));
 t.fish[0].x=1300;advisor_core_encode(&t,0,encoded,sizeof encoded);assert(strstr(encoded,"wall near 12"));
 t.fish[0].x=1000;t.fish[0].goal.id=GOAL_EXPLORE;t.fish[0].goal.urgency=2;
 int beyond=0;
 for(int k=0;k<1800;k++){tank_tick(&t,1.0f/30,NULL);for(int i=0;i<t.n_fish;i++){assert(isfinite(t.fish[i].x));assert(t.fish[i].x>=0 && t.fish[i].x<TANK_W);assert(t.fish[i].y>=0 && t.fish[i].y<TANK_H);beyond+=t.fish[i].x>448;}}
 assert(beyond>0);
 setup_begin(&t);setup_activate(&t,SETUP_HIT_NEXT);assert(setup_page()==SETUP_PG_BUBBLES);
 setup_touch(&t,1200,364,true);setup_touch(&t,1200,364,false);assert(t.bubble_x>1100);
 /* New items buy/place/save/reload/sell without shifting legacy fields. */
 setup_cancel(&t); t.sd_balance=1000;
 for(int item=EXTRA_DECOR_FIRST;item<SD_ITEM_COUNT;item++) {
  int before=t.sd_balance; assert(progression_buy(&t,item));
  assert(t.sd_balance==before-SD_ITEMS[item].price);
  assert(!progression_buy(&t,item));
  tank_decor_set(&t,item,1100+(item-6)*30,DECOR_Z_FRONT);
 }
 assert(progression_save(&t)); tank_t loaded; tank_init(&loaded,55); progression_boot(&loaded);
 assert(loaded.sd_balance==t.sd_balance);
 for(int i=0;i<EXTRA_DECOR_COUNT;i++) {
  assert(loaded.extra_decor_x[i]==t.extra_decor_x[i]);
  assert(loaded.extra_decor_z[i]==DECOR_Z_FRONT);
 }
 for(int i=0;i<N_FISH_MAX;i++) assert(loaded.fish_species[i]==SPECIES_CLASSIC);
 assert(TANK_SPECIES[SPECIES_CLOWN].marine && !TANK_SPECIES[SPECIES_PLECO].marine);
 for(int item=EXTRA_DECOR_FIRST;item<SD_ITEM_COUNT;item++) {
  int before=loaded.sd_balance;
  assert(progression_sell(&loaded,item));
  assert(loaded.sd_balance==before+progression_sell_value(item));
  assert(!(loaded.sd_unlocks & (1u<<item)));
 }
 /* Reach every new item through the actual paged shop, then move it. */
 for(int item=EXTRA_DECOR_FIRST;item<SD_ITEM_COUNT;item++) {
  render_shop_leave();
  for(int page=0;page<item/4;page++) assert(render_shop_tap(&t,400,35)==SHOP_TAP_KEPT);
  assert(render_shop_tap(&t,100,98+(item%4)*56)==SHOP_TAP_KEPT);
  assert(render_shop_focus()>=0);
  assert(render_shop_tap(&t,140,265)==SHOP_TAP_MOVE+item);
 }
 assert(!tank_set_species(&loaded,-1,SPECIES_PLECO));
 assert(!tank_set_species(&loaded,0,SPECIES_COUNT));
 for(int species=0;species<SPECIES_COUNT;species++) {
  assert(tank_set_species(&loaded,0,species)); assert(progression_save(&loaded));
  tank_t again; tank_init(&again,29); progression_boot(&again);
  assert(again.fish_species[0]==species && again.fish[0].species==species);
  assert(!strcmp(again.fish[0].name,loaded.fish[0].name));
  assert(again.sd_balance==loaded.sd_balance && again.fish[0].ms_bits==loaded.fish[0].ms_bits);
 }
 tank_t pleco;tank_init(&pleco,67); tank_new_population(&pleco); assert(tank_set_species(&pleco,0,SPECIES_PLECO));
 memset(pleco.algae,0,sizeof pleco.algae);
 int cell=10*ALGAE_COLS+12;pleco.algae[cell]=3;
 pleco.fish[0].x=(12.5f)*ALGAE_CELL_X;pleco.fish[0].y=10.5f*ALGAE_CELL;
 pleco.fish[0].hunger=1;pleco.fish[0].goal.id=GOAL_REST;
 for(int k=0;k<220;k++) tank_tick(&pleco,1.0f/30,NULL);
 assert(pleco.fish[0].glass_attached && pleco.algae[cell]<3);
 pleco.fish[0].goal.id=GOAL_SEEK_FOOD; tank_tick(&pleco,.03f,NULL); assert(!pleco.fish[0].glass_attached);
 pleco.fish[0].goal.id=GOAL_REST;pleco.hold_active=true; tank_tick(&pleco,.03f,NULL); assert(!pleco.fish[0].glass_attached);
 pleco.fish[0].goal.id=GOAL_FLEE_SHADOW; tank_tick(&pleco,.03f,NULL); assert(!pleco.fish[0].glass_attached);
 for(int h=0;h<HABITAT_COUNT;h++) {
  loaded.habitat=h; assert(progression_save(&loaded));
  tank_t again; tank_init(&again,29); progression_boot(&again);
  assert(again.habitat==h && again.sd_balance==loaded.sd_balance);
  assert(!memcmp(again.fish_species,loaded.fish_species,sizeof loaded.fish_species));
  assert(again.sd_unlocks==loaded.sd_unlocks);
 }
 for(int h=0;h<HABITAT_COUNT;h++) for(int sp=0;sp<SPECIES_COUNT;sp++)
  assert(tank_species_allowed(h,sp)==(h==0 || sp==0 || (h==2?sp==SPECIES_CLOWN:sp!=SPECIES_CLOWN)));
 puts("PASS habitat save persistence, fish/purchase retention and species grouping");
 puts("PASS species persistence, identity preservation, pleco grazing and release priorities");
 puts("PASS expansion purchase, duplicate prevention, placement, save, reload, refund and species defaults");
 puts("PASS: wide movement bounds, right-side feeding and placement, six model zones, unchanged distance/direction units");
 return 0;
}
