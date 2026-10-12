/* Production renderer is included so assertions can inspect the modal's
 * private presentation state, without adding a device-only testing API. */
#include "../../common/render.c"
#include "advisor.h"
#include <assert.h>
bool persist_port_save(const void *p,size_t n){(void)p;(void)n;return true;}
bool persist_port_load(void *p,size_t n,size_t *got){(void)p;(void)n;(void)got;return false;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return 1800000000;}
const char *version_port_string(void){return "HOST";}
static void ready(tank_t *t){
    tank_init(t,19);progression_fresh(t);tank_habitat_defaults(t,HABITAT_FRESH);
    progression_setup_done(t);t->ui_cover=true;t->light_override=true;t->light_on=true;
    for(int i=0;i<2;i++){
        tank_set_species(t,i,SPECIES_TIGER_BARB);progression_set_age(t,i,37.2f*3600);
        t->fish[i].hunger=2;t->fish[i].energy=8;t->fish[i].stress=0;
        t->fish[i].goal.id=GOAL_VISIT_BUBBLES;t->fish[i].goal_age=20;t->fish[i].ask_age=50;
    }
    tank_set_name(t,0,"ripple");tank_set_name(t,1,"willow");
}
static goal_t stale_bubbles(const tank_t *t,int i,bool request){
    (void)t;(void)i;(void)request;
    return (goal_t){GOAL_VISIT_BUBBLES,3,1,GOAL_COUNT};
}
static void recovery(void){
    tank_t t;ready(&t);t.fish[1].energy=.1f;
    tank_tick(&t,.1f,stale_bubbles);
    assert(t.fish[1].goal.id==GOAL_REST && t.fish[0].goal.id==GOAL_VISIT_BUBBLES);
    int frames=0;
    while(t.fish[1].energy<6 && frames++<300){
        tank_tick(&t,.1f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_REST);
    }
    assert(t.fish[1].energy>=6);
    tank_tick(&t,.1f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_VISIT_BUBBLES);
    for(int k=0;k<50;k++){tank_tick(&t,.1f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_VISIT_BUBBLES);}
    /* Restore the existing persisted REST goal: recovery survives a snapshot
     * without introducing a latch in the save format. */
    tank_t saved=t;saved.fish[1].energy=3;saved.fish[1].goal.id=GOAL_REST;
    t=saved;tank_tick(&t,.1f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_REST);
    t.fish[1].energy=1.51f;t.fish[1].goal.id=GOAL_EXPLORE;
    tank_tick(&t,.01f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_VISIT_BUBBLES);
    t.fish[1].energy=1.5f;tank_tick(&t,.01f,stale_bubbles);assert(t.fish[1].goal.id==GOAL_REST);
    puts("PASS recovery entry/release, stale-response veto, no oscillation, per-fish independence, restored REST goal");
}
static void care_and_modal(uint16_t *fb){
    tank_t t;ready(&t);fry_req_t r[FRY_REQ_MAX];bool staged;int a,b;
    t.fish[1].hunger=5.6f;t.fish[1].energy=.1f;
    assert(!progression_breed_pair(&t,&a,&b));
    assert(progression_next_fry(&t,r,&staged)==3);
    assert(strstr(r[0].words,"willow: HUNGRY/TIRED"));assert(strstr(r[0].progress,"PAUSED"));assert(r[1].met);
    ms_open(&t,t.n_fish,false,true,0,r,3,staged);
    render_milestones(&t,fb,TANK_W);
    FILE *image=fopen("care-blockers.ppm","wb");assert(image);fprintf(image,"P6\n%d %d\n255\n",TANK_UI_W,TANK_H);
    for(int y=0;y<TANK_H;y++)for(int x=0;x<TANK_UI_W;x++){uint16_t c=fb[y*TANK_W+x];unsigned char rgb[3]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(rgb,1,3,image);}fclose(image);
    for(int i=0;i<2;i++){t.fish[i].hunger=4;t.fish[i].energy=4;t.fish[i].stress=3;}
    assert(progression_breed_pair(&t,&a,&b));
    t.fish[1].hunger=4.01f;assert(!progression_breed_pair(&t,&a,&b));t.fish[1].hunger=4;
    t.fish[1].energy=3.99f;assert(!progression_breed_pair(&t,&a,&b));t.fish[1].energy=4;
    progression_context_t c;progression_context_capture(&c);c.s_breed_ready=59;c.s_breed_cool=0;progression_context_restore(&c);
    render_milestones(&t,fb,TANK_W);assert(!strcmp(g_ms_sub,"0 / 15 MIN"));
    g_ms_tip=true;int row=g_ms_row,k=g_ms_k;
    progression_tick(&t,1);render_milestones(&t,fb,TANK_W);
    assert(!strcmp(g_ms_sub,"1 / 15 MIN")&&g_ms_tip&&g_ms_row==row&&g_ms_k==k);
    t.fish[1].stress=3.01f;progression_tick(&t,.1f);render_milestones(&t,fb,TANK_W);
    assert(strstr(g_ms_caption,"STRESSED")&&strstr(g_ms_sub,"1 / 15 MIN - PAUSED")&&g_ms_tip);
    t.fish[1].stress=0;t.fish[1].stage=STAGE_JUV;
    progression_next_fry(&t,r,&staged);assert(strstr(r[0].words,"NEEDS TO GROW"));
    t.fish[1].stage=STAGE_ELDER;tank_set_species(&t,1,SPECIES_GUPPY);
    progression_next_fry(&t,r,&staged);assert(strstr(r[0].words,"MATCHING SPECIES"));
    tank_set_species(&t,1,SPECIES_TIGER_BARB);t.habitat=HABITAT_REEF;
    progression_next_fry(&t,r,&staged);assert(strstr(r[0].words,"NOT FOR THIS TANK"));t.habitat=HABITAT_FRESH;
    progression_context_capture(&c);c.s_breed_cool=61;progression_context_restore(&c);
    progression_next_fry(&t,r,&staged);assert(strstr(r[0].words2,"COOLDOWN")&&strstr(r[2].progress,"2 MIN"));
    ms_open(&t,t.n_fish,false,true,2,r,3,staged);progression_tick(&t,1);render_milestones(&t,fb,TANK_W);
    assert(!strcmp(g_ms_sub,"1 MIN REMAIN")&&g_ms_k==2);
    c.s_breed_cool=0;progression_context_restore(&c);
    for(int bed=0;bed<tank_veg_beds(&t);bed++)tank_veg_set(&t,bed,VEG_NUB);
    progression_next_fry(&t,r,&staged);assert(!r[1].met&&strstr(r[0].words2,"NURSERY COVER"));
    t.n_fish=POP_CAP;assert(progression_next_fry(&t,r,&staged)==0);
    puts("PASS exact care boundaries/blockers, matching/age/habitat/cover/cooldown, live minute/reset, HOW and navigation preservation");
}
static void portraits(uint16_t *fb){
    tank_t t;ready(&t);size_t bytes=(size_t)TANK_W*TANK_H*2;
    uint16_t *expected=calloc(1,bytes);assert(expected);
    for(int species=0;species<SPECIES_COUNT;species++)for(int stage=0;stage<4;stage++){
        fish_t f=t.fish[0];f.species=species;f.stage=stage;f.size=.6f+stage*.2f;
        f.color=0x2378bc;f.fin=0xcfa241;f.accent=0x73d827;
        memset(fb,0,bytes);memset(expected,0,bytes);
        render_fish_portrait(fb,TANK_W,100,100,f.size,&f,1);
        f.x=100;f.y=100;f.heading=0;f.speed=40;f.wander=0;f.hunger=4;f.stress=0;f.goal.id=GOAL_EXPLORE;tank_fish_face(&f);
        ctx_t context=ctx_full(expected,TANK_W,1);draw_fish_core(&context,&f,1,false,f.size);
        assert(!memcmp(fb,expected,bytes));
        memset(fb,0,bytes);render_fish_silhouette(fb,TANK_W,100,100,f.size,&f,1,0x2a3f45);
        int pixels=0;for(size_t i=0;i<bytes/2;i++)if(fb[i]){pixels++;assert(fb[i]==rgb565(0x2a3f45,1));}assert(pixels>0);
    }
    fish_t f=t.fish[0];f.species=SPECIES_CLASSIC;f.stage=STAGE_ADULT;
    memset(fb,0,bytes);memset(expected,0,bytes);
    render_fish_preview(fb,TANK_W,100,100,1,f.color,f.fin,f.accent,1);
    render_fish_portrait(expected,TANK_W,100,100,1,&f,1);assert(!memcmp(fb,expected,bytes));
    /* Exercise the real milestone row, not only its portrait helper. */
    render_milestones_leave();render_milestones(&t,fb,TANK_W);
    for(size_t i=0;i<bytes/2;i++)expected[i]=rgb565(MSP_INK,1);
    render_fish_portrait(expected,TANK_W,52,MSP_ROW_Y0+20,t.fish[0].size,&t.fish[0],t.clock);
    for(int y=MSP_ROW_Y0;y<MSP_ROW_Y0+MSP_ROW_H;y++)for(int x=0;x<90;x++)
        assert(fb[y*TANK_W+x]==expected[y*TANK_W+x]);
    free(expected);puts("PASS 20 species x 4 stages match aquarium core, locked single-ink silhouettes, classic palette preview preserved");
}

static progression_context_t context(void){progression_context_t c;progression_context_capture(&c);return c;}
static void seconds(tank_t *t,int n){while(n--)progression_tick(t,1);}
static void accumulated_care(void){
 tank_t t;ready(&t);seconds(&t,120);assert(context().s_breed_ready==120);
 t.fish[1].hunger=5.6f;seconds(&t,600);assert(context().s_breed_ready==120);
 t.fish[1].hunger=2;t.fish[1].energy=1;seconds(&t,60);assert(context().s_breed_ready==120);
 t.fish[1].energy=8;seconds(&t,60);assert(context().s_breed_ready==180);
 t.fish[1].hunger=9;seconds(&t,300);assert(context().s_breed_ready==180);
 progression_context_t save=context();tank_t saved=t;
 ready(&t);seconds(&t,20);assert(context().s_breed_ready==20);
 t=saved;progression_context_restore(&save);seconds(&t,50);assert(fabsf(context().s_breed_ready-170)<.01f);
 t.fish[1].hunger=2;seconds(&t,30);assert(fabsf(context().s_breed_ready-200)<.01f && context().s_breed_bad_cs==0);
 for(int bed=0;bed<tank_veg_beds(&t);bed++)tank_veg_set(&t,bed,VEG_NUB);
 seconds(&t,100);assert(fabsf(context().s_breed_ready-200)<.01f);
 tank_set_species(&t,0,SPECIES_GUPPY);tank_set_species(&t,1,SPECIES_GUPPY);
 seconds(&t,1);assert(context().s_breed_ready==0);
 ready(&t);save=context();save.s_breed_ready=42;save.s_breed_state_version=0;save.s_breed_pair=0;
 progression_context_restore(&save);seconds(&t,1);assert(context().s_breed_ready==43);
 seconds(&t,1000);assert(context().s_breed_ready==900);
 t.ui_cover=false;seconds(&t,1);t.spawning=true;t.spawn_danced=SPAWN_DANCE_S;
 progression_sd_take_award();seconds(&t,1);assert(t.n_fish==3 && progression_sd_take_award()==40);
 assert(context().s_breed_cool==BREED_COOLDOWN_S && context().s_breed_ready==0);
 save=context();save.s_breed_cool=21600;progression_context_restore(&save);seconds(&t,60);
 assert(context().s_breed_ready==0 && context().s_breed_cool==21540);
 ready(&t);int interrupted=0;float peak=0;
 for(int k=0;k<36000;k++) {tank_tick(&t,.1f,advisor_rules);progression_tick(&t,.1f);
  float v=context().s_breed_ready;if(v>peak)peak=v;
  int a,b;if(!progression_breed_pair(&t,&a,&b))interrupted++;
 }
 printf("Natural rules auto-care, 1 active hour: peak accumulated %.2fs, interrupted frames %d\n",peak,interrupted);
 assert(peak>300 && interrupted>0);
 puts("PASS accumulated care, hunger/energy/cover pause, severe grace/decay/recovery, context switching/reload, pair invalidation, old padding, cap/cooldown");
}
static void doubled_rewards(void){
 tank_t t;ready(&t);seconds(&t,1);progression_sd_take_award();t.sd_balance=t.sd_earned=0;
 t.player_feedings++;seconds(&t,1);assert(t.sd_balance==4 && progression_sd_take_award()==4);
 seconds(&t,1);assert(t.sd_balance==4 && progression_sd_take_award()==0);
 t.algae_colonies+=100;seconds(&t,1);assert(progression_sd_take_award()==50);
 t.trim_px+=PX_PER_CM*250;seconds(&t,1);assert(progression_sd_take_award()==50);
 t.fish[0].trust=10;seconds(&t,1);assert(progression_sd_take_award()==30);
 t.sd_paid_fish[0]=0;seconds(&t,1);assert(progression_sd_take_award()==110); /* 10+20+50+30 */
 progression_sd_grant(&t,7);assert(progression_sd_take_award()==7);
 int before=t.sd_balance;t.sd_unlocks|=SD_ITEM_CASTLE;assert(progression_sell(&t,2));
 assert(t.sd_balance-before==30 && progression_sd_take_award()==30);
 t.sd_balance=INT32_MAX-2;t.sd_earned=INT32_MAX-1;t.player_feedings++;seconds(&t,1);
 assert(t.sd_balance==INT32_MAX && t.sd_earned==INT32_MAX && progression_sd_take_award()==2);
 t.player_feedings++;seconds(&t,1);assert(progression_sd_take_award()==0);
 progression_sd_grant(&t,INT32_MIN);assert(t.sd_balance==0);
 const char *const *lines=progression_sd_earn_lines();assert(strstr(lines[0],"+4 ")&&strstr(lines[1],"+10/20/50")&&strstr(lines[2],"+40 "));
 progression_context_t c=context();int32_t bal=t.sd_balance,earned=t.sd_earned;progression_context_restore(&c);
 assert(t.sd_balance==bal && t.sd_earned==earned);
 uint16_t *fb=calloc(TANK_W*TANK_H,2);assert(fb);t.clock=10;t.sd_balance=0;
 progression_sd_grant(&t,INT32_MAX);render_sd_toast(&t,fb,TANK_W);t.sd_balance=0;
 progression_sd_grant(&t,100);render_sd_toast(&t,fb,TANK_W);assert(g_toast_n==INT32_MAX);free(fb);
 puts("PASS 2x meal/stages/trust/chores, no duplicate award, unchanged grant/refund, cap/overflow, reward labels");
}
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    uint16_t *fb=calloc((size_t)TANK_W*TANK_H,2);assert(fb);
    accumulated_care();doubled_rewards();recovery();care_and_modal(fb);portraits(fb);free(fb);return 0;
}
