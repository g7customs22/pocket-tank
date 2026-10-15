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
#include "game_clock.h"
#include "../../common/render.c"
static void healthy(tank_t *t){tank_init(t,23);progression_fresh(t);progression_setup_done(t);t->habitat=HABITAT_FRESH;t->light_auto=false;t->player_feedings=3;
 for(int i=0;i<2;i++){tank_set_species(t,i,SPECIES_NEON);progression_set_age(t,i,40000);t->fish[i].hunger=2;t->fish[i].energy=8;t->fish[i].stress=0;}for(int i=0;i<tank_veg_beds(t);i++)t->veg_growth[i]=1;}
static unsigned drain(game_clock_t *c,tank_t *t){unsigned n=game_clock_take(c);assert(n<=8);for(unsigned i=0;i<n;i++){wp_tick(t,&t->world,GAME_CLOCK_STEP,advisor_rules);progression_tick(t,GAME_CLOCK_STEP);}return n;}
static void timing(void){
 tank_t reference,stalled,t;progression_context_t expected,actual;game_clock_t c;unsigned ticks=0;
 healthy(&reference);game_clock_init(&c,0,0);
 for(int64_t us=10000;us<=10000000;us+=10000){game_clock_account(&c,us);ticks+=drain(&c,&reference);}assert(ticks==300);progression_context_capture(&expected);
 healthy(&stalled);game_clock_init(&c,0,0);ticks=0;
 /* Two seconds of silence, another eight-second stall; all owed steps remain. */
 game_clock_account(&c,2000000);ticks+=drain(&c,&stalled);assert(game_clock_due(&c));
 game_clock_account(&c,10000000);while(game_clock_due(&c))ticks+=drain(&c,&stalled);assert(ticks==300);progression_context_capture(&actual);
 assert(reference.clock==stalled.clock&&expected.s_breed_ready==actual.s_breed_ready&&expected.s_age[0]==actual.s_age[0]);
 assert(reference.sd_balance==stalled.sd_balance&&reference.sd_earned==stalled.sd_earned&&reference.veg_growth[0]==stalled.veg_growth[0]);
 tank_t slots[3];progression_context_t contexts[3];float ages[3];
 for(int i=0;i<3;i++){healthy(&slots[i]);ages[i]=progression_age_s(&slots[i],0);progression_context_capture(&contexts[i]);}
 game_clock_t banks;game_clock_init(&banks,0,0);int64_t stamps[]={2000000,10000000,11000000};unsigned owners[]={0,1,0};
 for(int i=0;i<3;i++){unsigned owner=owners[i];game_clock_resume(&banks,i?stamps[i-1]:0,owner);progression_context_restore(&contexts[owner]);game_clock_account(&banks,stamps[i]);while(game_clock_due(&banks))drain(&banks,&slots[owner]);progression_context_capture(&contexts[owner]);}
 assert(fabsf(slots[0].clock-3)<.001f&&fabsf(slots[1].clock-8)<.001f&&slots[2].clock==0);
 assert(contexts[0].s_breed_ready>2.99f&&contexts[1].s_breed_ready>7.99f&&contexts[2].s_breed_ready==0);
 assert(slots[2].sd_balance==0&&contexts[2].s_age[0]==ages[2]);
 healthy(&t);game_clock_t slow;game_clock_init(&slow,0,0);unsigned slow_ticks=0;
 for(int64_t us=200000;us<=10000000;us+=200000){game_clock_account(&slow,us);while(game_clock_due(&slow))slow_ticks+=drain(&slow,&t);}
 assert(slow_ticks==300&&t.sd_balance==stalled.sd_balance&&t.sd_earned==stalled.sd_earned&&t.clock==stalled.clock&&t.sd_earned>0);
 int balance=stalled.sd_balance;game_clock_account(&c,9000000);game_clock_account(&c,10000000);assert(!drain(&c,&stalled)&&stalled.sd_balance==balance);
 /* Earned fractional time remains with its tank. Transition/sleep gaps excluded. */
 healthy(&t);game_clock_init(&c,0,0);game_clock_account(&c,20000);assert(!game_clock_take(&c));
 game_clock_resume(&c,5000000,1);game_clock_account(&c,7000000);unsigned one=0;while(game_clock_due(&c))one+=game_clock_take(&c);assert(one==60&&c.debt[0]==600000&&c.debt[2]==0);
 game_clock_resume(&c,9000000,0);game_clock_account(&c,9013334);assert(game_clock_take(&c)==1&&c.debt[1]==0&&c.debt[0]==20);
 game_clock_resume(&c,36009013334LL,0);game_clock_account(&c,36009013334LL);assert(!game_clock_take(&c)&&c.debt[0]==20);
 game_clock_account(&c,36010013334LL);unsigned after=0;while(game_clock_due(&c))after+=game_clock_take(&c);assert(after==30);
 game_clock_reset_active(&c,36010013334LL);assert(!c.debt[0]);
 puts("PASS 2s/8s stalls vs 10ms pacing: exact 300 ticks, equal growth/care/age/rewards; no duplicate credit; bounded batches; slot ownership; sleep and transition gaps excluded");
}
static void frame(tank_t*t,uint16_t*src,uint16_t*dst,int key,int scroll,const char*name){ui349_state_t ui={0};ui349_configure(&ui,(ui349_view_t){key,"MENU",true,scroll});ui349_render(&ui,t,src,dst);ppm(name,dst);}
int main(void){setvbuf(stdout,NULL,_IONBF,0);timing();tank_t t;healthy(&t);uint16_t*src=calloc(TANK_W*TANK_H,2),*dst=calloc(640*172,2);assert(src&&dst);
 /* Every catalog item is checked for text width, including habitat-specific names. */
 for(int habitat=0;habitat<3;habitat++){t.habitat=habitat;for(int item=0;item<SD_ITEM_COUNT;item++)if(tank_item_allowed(habitat,item)){
 const sd_item_t *it=shop_info(&t,item);if(text_w(it->name,3)>SHP_MODAL_W-8)printf("WIDE TITLE %d %s width%d\n",item,it->name,text_w(it->name,3));
 assert(text_w(it->words,2)<=SHP_MODAL_W-8&&text_w(it->words2,2)<=SHP_MODAL_W-8);
 }}
 t.habitat=HABITAT_REEF;t.sd_balance=0;g_shp_modal=1;render_shop(&t,src,TANK_W);frame(&t,src,dst,2348,render_shop_focus(),"shop-snail.ppm");frame(&t,src,dst,2348,140,"shop-snail-bottom.ppm");
 g_shp_modal=2;t.sd_unlocks|=SD_ITEM_CASTLE;assert(render_shop_tap(&t,290,260)==SHOP_TAP_KEPT&&g_shp_sell_armed);render_shop(&t,src,TANK_W);frame(&t,src,dst,2348,120,"shop-sell-confirm.ppm");assert(render_shop_tap(&t,290,260)==SHOP_TAP_SELL+2);
 render_shop_leave();render_shop(&t,src,TANK_W);frame(&t,src,dst,300,0,"shop-list-top.ppm");frame(&t,src,dst,300,196,"shop-list-bottom.ppm");
 strcpy(g_ms_title,"PAIR + CARE");strcpy(g_ms_caption,"CHECK CARE");g_ms_kind=FRY_REQ_GROW;g_ms_tip=true;render_milestones(&t,src,TANK_W);frame(&t,src,dst,2460,60,"breeding-help-top.ppm");frame(&t,src,dst,2460,130,"breeding-help-bottom.ppm");
 render_milestones_leave();render_confirm_reset(src,TANK_W,.75f);frame(&t,src,dst,1000,76,"reset-confirm.ppm");frame(&t,src,dst,1000,120,"reset-confirm-bottom.ppm");
 ui349_state_t u={0};u.copy_picker=true;u.copy_item=8;u.copy_selected=2;u.copy_sell_armed=true;ui349_render(&u,&t,src,dst);ppm("copy-sell-confirm.ppm",dst);
 free(src);free(dst);puts("PASS shop/help/confirmation frame generation and sale two-tap behavior");return 0;
}

