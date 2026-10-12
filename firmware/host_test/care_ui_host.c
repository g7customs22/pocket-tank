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
    assert(strstr(r[0].words,"willow: HUNGRY/TIRED"));assert(strstr(r[0].progress,"BLOCKED"));assert(r[1].met);
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
    assert(strstr(g_ms_caption,"STRESSED")&&strstr(g_ms_sub,"0 / 15 MIN - BLOCKED")&&g_ms_tip);
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
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);
    uint16_t *fb=calloc((size_t)TANK_W*TANK_H,2);assert(fb);
    recovery();care_and_modal(fb);portraits(fb);free(fb);return 0;
}
