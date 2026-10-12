/* Isolated production-renderer scenarios; no serial, filesaves or physical tank. */
#define main care_regression_main
#include "care_ui_host.c"
#undef main
#include "ui_349_v1.h"
static void shot(const char *name,ui349_state_t *u,tank_t *t,uint16_t *fb){
 uint16_t out[640*172]={0};if(u->view.key>=700&&u->view.key<700+N_FISH_MAX){uint16_t *world=calloc(TANK_W*TANK_H,2);render_tank(t,world,TANK_W);ui349_capture_fish(u,t,world,out);free(world);}ui349_render(u,t,fb,out);char path[160];snprintf(path,sizeof path,"outputs/ui-review/%s.ppm",name);
 FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n640 172\n255\n");
 for(int i=0;i<640*172;i++){uint16_t c=out[i];unsigned char rgb[3]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
static int tap(ui349_state_t *u,int x,int y){bool down;int tx,ty;ui349_input(u,true,x,y,&down,&tx,&ty);return ui349_input(u,false,x,y,&down,&tx,&ty);}
int main(void){
 uint16_t *fb=calloc(TANK_W*TANK_H,2);tank_t t;ready(&t);ui349_state_t u={0};u.battery.mv=3900;u.battery.pct=60;
 render_milestones_leave();render_milestones(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){400,"MENU",true,0});shot("menu-top",&u,&t,fb);
 int taps=0;while(u.scroll<UI349_SCROLL_MAX){tap(&u,40,135);taps++;}shot("menu-bottom",&u,&t,fb);printf("Menu top to bottom: %d DOWN taps, %d pixels\n",taps,u.scroll);
 fry_req_t r[FRY_REQ_MAX];bool staged;progression_context_t c=context();c.s_breed_ready=390;progression_context_restore(&c);
 for(int scenario=0;scenario<4;scenario++){
 t.fish[1].hunger=scenario==1?5.6f:scenario==2?9:2;t.fish[1].energy=scenario==1?.1f:8;
 c=context();c.s_breed_ready=scenario==3?900:390;c.s_breed_bad_cs=scenario==2?30000:0;progression_context_restore(&c);
 progression_next_fry(&t,r,&staged);ms_open(&t,t.n_fish,false,true,0,r,3,staged);render_milestones(&t,fb,TANK_W);
 ui349_configure(&u,(ui349_view_t){2400+scenario,"MENU",true,render_milestones_focus()});
 const char *names[]={"care-healthy","care-paused","care-decay","care-ready"};shot(names[scenario],&u,&t,fb);
 }
 ms_open(&t,0,false,false,0,r,3,false);render_milestones(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){2898,"MENU",true,render_milestones_focus()});shot("milestone-detail",&u,&t,fb);
 ms_open(&t,t.n_fish,false,true,0,r,3,staged);g_ms_tip=true;render_milestones(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){2999,"MENU",true,render_milestones_focus()});shot("care-how-top",&u,&t,fb);u.scroll=UI349_SCROLL_MAX;shot("care-how-bottom",&u,&t,fb);
 render_milestones_leave();render_shop_leave();t.sd_balance=4;render_shop(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){300,"SHOP",true,0});shot("shop-poor",&u,&t,fb);
 g_shp_modal=2;render_shop(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){2300,"SHOP",true,render_shop_focus()});shot("shop-unaffordable",&u,&t,fb);
 assert(render_shop_tap(&t,224,SHP_MODAL_Y+SHP_MODAL_H-20)==SHOP_TAP_KEPT);assert(g_shp_modal==-1);puts("Confirmed: tapping disabled UNLOCK dismisses item modal without explaining shortfall");g_shp_modal=2;
 t.sd_balance=150;render_shop(&t,fb,TANK_W);shot("shop-affordable",&u,&t,fb);
 g_shp_modal=-1;g_shp_earn=true;render_shop(&t,fb,TANK_W);ui349_configure(&u,(ui349_view_t){2399,"SHOP",true,render_shop_focus()});shot("rewards",&u,&t,fb);
 ui349_configure(&u,(ui349_view_t){701,"FISH",true,0});render_tank(&t,fb,TANK_W);render_stats_card(&t,1,fb,TANK_W);shot("fish-detail",&u,&t,fb);
 u.species_picker=true;u.habitat=HABITAT_FRESH;shot("species",&u,&t,fb);assert(tap(&u,150,30)>=UI349_SPECIES_PICK);
 u.species_picker=false;u.slots_picker=true;u.active_slot=1;u.slots_present=7;u.slot_fish[0]=u.slot_fish[1]=u.slot_fish[2]=2;shot("tank-switcher",&u,&t,fb);
 assert(tap(&u,200,82)==UI349_SLOT_PICK+1);assert(tap(&u,590,30)==UI349_TANKS_BACK);
 puts("PASS native 640x172 scenarios, scroll and picker touch actions");free(fb);return 0;
}
