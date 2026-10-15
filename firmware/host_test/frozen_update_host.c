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
#include "viewport_349_v1.h"
#include "render_cadence.h"
#include "advisor.h"
#include <math.h>
static void ppm(const char *name,uint16_t *fb){FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n640 172\n255\n");for(int i=0;i<640*172;i++){unsigned p=fb[i];unsigned char rgb[3]={(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
int main(void){
 const int levels[]={5,10,30,60,100};
 for(int i=0;i<5;i++){int value;assert(render_settings_tap(212+i*49,65,&value)==SET_TAP_BRIGHT&&value==levels[i]);if(i)assert(v1_image_gain(levels[i])>v1_image_gain(levels[i-1]));assert(v1_backlight_drive(levels[i]*255/100)>0);}
 ui349_state_t s={0};bool down;int x,y;
 s.wake_pending=true;assert(!ui349_wake_filter(&s,false,false));assert(ui349_wake_filter(&s,true,false));assert(ui349_wake_filter(&s,true,false));assert(ui349_wake_filter(&s,false,false));assert(!ui349_wake_filter(&s,true,false));
 assert(ui349_wake_filter(&s,true,true));assert(ui349_wake_filter(&s,false,false));

 for(int key=300;key<=400;key+=100){
  memset(&s,0,sizeof s);ui349_configure(&s,(ui349_view_t){key,"PAGE",true,0});
  assert(ui349_input(&s,true,300,140,&down,&x,&y)==UI349_PASS&&down);
  assert(ui349_input(&s,true,302,30,&down,&x,&y)==UI349_CONSUMED&&!down&&s.scroll==110);
  assert(ui349_input(&s,false,302,30,&down,&x,&y)==UI349_CONSUMED&&!down&&!s.owner);
  assert(ui349_input(&s,true,20,135,&down,&x,&y)==UI349_CONSUMED);
  assert(ui349_input(&s,false,20,135,&down,&x,&y)==UI349_CONSUMED&&s.scroll==150);
 }
 ui349_configure(&s,(ui349_view_t){200,"OPTIONS",true,0});
 ui349_input(&s,true,300,140,&down,&x,&y);assert(ui349_input(&s,true,300,30,&down,&x,&y)==UI349_PASS&&down&&s.scroll==0);ui349_input(&s,false,300,30,&down,&x,&y);
 s.history_picker=true;assert(!ui349_map(&s,300,50,&x,&y));assert(ui349_input(&s,true,300,50,&down,&x,&y)==UI349_CONSUMED);assert(ui349_input(&s,false,300,50,&down,&x,&y)==UI349_CONSUMED);
 assert(!render_due(32000,1,true,false)&&render_due(33334,1,true,false));assert(!render_due(120000,1,false,false)&&render_due(125001,1,false,false));assert(render_interval_us(false,true)==200000);
 tank_t t;uint16_t *src=calloc(TANK_W*TANK_H,2),*buf=calloc(640*172+2,2);assert(src&&buf);buf[0]=0x1234;buf[640*172+1]=0xabcd;
 tank_init(&t,23);progression_fresh(&t);progression_setup_done(&t);notice_sync(&t);
 int count=notice_history_count();notice_tick(&t,.1f,false);assert(notice_history_count()==count);
 notice_low_battery();notice_low_battery();assert(notice_history_count()==count+1);notice_tick(&t,.1f,true);notice_tick(&t,.1f,false);assert(notice_history_count()==count+1);
 for(int i=0;i<8;i++){notice_sync(&t);notice_low_battery();}assert(notice_history_count()==NOTICE_HISTORY);
 s=(ui349_state_t){0};ui349_configure(&s,(ui349_view_t){400,"MENU",true,0});s.history_picker=true;ui349_render(&s,&t,src,buf+1);ppm("history.ppm",buf+1);
 s.history_picker=false;ui349_configure(&s,(ui349_view_t){200,"OPTIONS",true,0});render_settings(&t,src,TANK_W,5,1);ui349_render(&s,&t,src,buf+1);ppm("brightness.ppm",buf+1);
 for(int h=0;h<3;h++){
  assert(tank_item_allowed(h,1));tank_init(&t,23);progression_fresh(&t);progression_setup_done(&t);t.habitat=h;t.sd_balance=1000;assert(progression_buy(&t,1));
  memset(t.algae,0,sizeof t.algae);int cell=8*ALGAE_COLS+12;t.algae[cell]=90;t.snail_cell=cell;t.snail_x=(12.5f)*ALGAE_CELL_X;t.snail_y=8.5f*ALGAE_CELL;
  int cleaned=t.cells_cleaned;for(int i=0;i<90;i++)wp_tick(&t,&t.world,1.f/30,advisor_rules);
  assert(!t.algae[cell]&&t.snail_grazed>=1&&t.cells_cleaned==cleaned);
 }

 assert(!tank_species_allowed(HABITAT_REEF,SPECIES_PLECO)&&!tank_species_allowed(HABITAT_REEF,SPECIES_BRISTLE));
 assert(tank_species_allowed(HABITAT_REEF,SPECIES_BLENNY)&&tank_species_allowed(HABITAT_REEF,SPECIES_OCTOPUS));
 assert(!tank_species_allowed(HABITAT_FRESH,SPECIES_OCTOPUS));
 tank_init(&t,23);progression_fresh(&t);progression_setup_done(&t);t.habitat=HABITAT_REEF;t.light_auto=false;t.light_manual_off=false;t.sd_balance=1000;assert(progression_buy(&t,1));
 tank_set_species(&t,0,SPECIES_BLENNY);tank_set_species(&t,1,SPECIES_OCTOPUS);
 for(int i=0;i<t.n_fish;i++){progression_set_age(&t,i,40000);t.fish[i].hunger=2;t.fish[i].energy=8;t.fish[i].stress=0;t.fish[i].x=640+i*65;t.fish[i].y=230;}
 wp_camera_t cam;wp_camera_init(&cam);cam.overview=true;
 for(int mode=0;mode<3;mode++){t.night=mode==2;t.uv.style=mode==1;render_world_proto(&t,&t.world,&cam,buf+1);char name[32];snprintf(name,sizeof name,"species-%d.ppm",mode);ppm(name,buf+1);}
 /* Rendering cadence never ticks gameplay: equal elapsed dt, unequal redraws. */
 for(int cadence=1;cadence<=12;cadence+=11){
  tank_init(&t,33);progression_fresh(&t);progression_setup_done(&t);t.habitat=HABITAT_REEF;t.light_auto=false;
  tank_set_species(&t,0,SPECIES_CLOWN);tank_set_species(&t,1,SPECIES_CLOWN);
  for(int i=0;i<2;i++){progression_set_age(&t,i,40000);t.fish[i].hunger=2;t.fish[i].energy=8;t.fish[i].stress=0;}
  for(int i=0;i<tank_veg_beds(&t);i++)t.veg_growth[i]=1;
  for(int i=0;i<100;i++){wp_tick(&t,&t.world,.1f,advisor_rules);progression_tick(&t,.1f);if(i%cadence==0)render_world_proto(&t,&t.world,&cam,buf+1);}
  progression_context_t context;progression_context_capture(&context);
  assert(fabsf(t.clock-10)<.01f&&context.s_breed_ready>9.9f);
  fry_req_t req[FRY_REQ_MAX];progression_next_fry(&t,req,NULL);assert(strstr(req[0].progress,":10")||strstr(req[0].progress,":09"));
 }
 assert(buf[0]==0x1234&&buf[640*172+1]==0xabcd);free(src);free(buf);
 puts("PASS brightness/hits; drag ownership/arrows/settings; bounded history/no replay; cadence-independent care; marine catalog; native species/UV/off guards");return 0;
}

