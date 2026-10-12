#include "slot_system.h"
#include "slot_journal.h"
#include "progression.h"
#include "esp_partition.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
static unsigned char flash[0x30000];
static const esp_partition_t partition={sizeof flash};
static int fail=-1,op;static int64_t wall=1800000000;
const esp_partition_t *esp_partition_find_first(int type,int sub,const char *name){assert(type==1&&sub==130&&!strcmp(name,"storage"));return &partition;}
int esp_partition_read(const esp_partition_t *p,uint32_t off,void *v,size_t n){assert(p==&partition&&off+n<=sizeof flash);memcpy(v,flash+off,n);return 0;}
int esp_partition_write(const esp_partition_t *p,uint32_t off,const void *v,size_t n){assert(p==&partition&&off>=0x10000&&off+n<=sizeof flash);if(op++==fail)return -1;for(size_t i=0;i<n;i++)flash[off+i]&=((const unsigned char *)v)[i];return 0;}
int esp_partition_erase_range(const esp_partition_t *p,uint32_t off,size_t n){assert(p==&partition&&off>=0x10000&&off+n<=sizeof flash);if(op++==fail)return -1;memset(flash+off,255,n);return 0;}
bool persist_port_save(const void *v,size_t n){(void)v;(void)n;return slot_system_enabled()?slot_system_save():true;}
bool persist_port_load(void *v,size_t n,size_t *got){(void)v;(void)n;(void)got;return false;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return wall;}
const char *version_port_string(void){return "HOST";}

/* Real growth/progression/slot code; only flash, clock and ESP allocation are mocked. */
static void cover(tank_t *t, float h) {
    for (int b=0;b<tank_veg_beds(t);b++) tank_veg_set(t,b,h);
}
static void nursery_check(const tank_t *t) {
    fry_req_t req[FRY_REQ_MAX]; bool staged;
    int n=progression_next_fry(t,req,&staged), found=0;
    for(int i=0;i<n;i++) if(req[i].kind==FRY_REQ_GRASS) {
        found++;
        printf("nursery: habitat=%u actual=%d checklist=%d progress=%s\n",
               t->habitat,tank_nursery_bed(t)>=0,req[i].met,req[i].progress);
        assert(req[i].met==(tank_nursery_bed(t)>=0));
        assert(req[i].frac>=0 && req[i].frac<=1);
    }
    assert(found==1);
}
int main(int argc,char **argv) {
    setvbuf(stdout,NULL,_IONBF,0);
    bool trace_only=argc>1 && !strcmp(argv[1],"--trace-only");
    static tank_t t, paused;
    memset(flash,255,sizeof flash);
    tank_init(&t,19); assert(slot_system_boot(&t)==0); progression_fresh(&t);
    assert(slot_system_migrate(&t));
    for(unsigned slot=0;slot<3;slot++) {
        assert(slot_system_switch(&t,slot));
        assert(tank_nursery_bed(&t)>=0); /* the seeded grass already qualifies */
        if(!trace_only) nursery_check(&t);
        cover(&t,VEG_NUB); assert(tank_nursery_bed(&t)<0);
        if(!trace_only) nursery_check(&t);
        float start=t.veg_growth[0];
        for(int i=0;i<180000;i++) tank_tick(&t,.1f,NULL); /* five active hours */
        printf("growth: slot=%u five awake hours %.6f -> %.6f\n",slot,start,t.veg_growth[0]);
        assert(fabsf(t.veg_growth[0]-(VEG_NUB+18000.0f/162000.0f))<.003f);
        assert(tank_nursery_bed(&t)>=0);
        paused=t; assert(slot_system_save());
        assert(slot_system_switch(&t,(slot+1)%3)); wall+=86400;
        assert(slot_system_switch(&t,slot));
        assert(!memcmp(paused.veg_h,t.veg_h,sizeof t.veg_h));
        assert(!memcmp(paused.veg_growth,t.veg_growth,sizeof t.veg_growth));
        puts("PASS: inactive day and slot roundtrip preserve exact plant heights");
        cover(&t,VEG_NUB); assert(slot_system_save()); wall+=7200;
        tank_init(&t,91); assert(slot_system_boot(&t)==1);
        assert(t.habitat==slot && fabsf(t.veg_growth[0]-.13f)<.0001f);
        assert(tank_nursery_bed(&t)>=0);
        puts("PASS: selected slot reboot applies two-hour offline plant growth");
        if(!trace_only) nursery_check(&t);
    }
    /* Fresh/reef default fish are unlike species: cover must not look missing. */
    for(unsigned slot=1;slot<3;slot++) {
        assert(slot_system_switch(&t,slot));
        int a,b; assert(!progression_breed_pair(&t,&a,&b));
        assert(tank_nursery_bed(&t)>=0);
        if(!trace_only) nursery_check(&t);
    }

    /* A grown, matching pair still owes the existing 15 active minutes. */
    tank_set_species(&t,1,t.fish[0].species);
    progression_set_age(&t,0,10800); progression_set_age(&t,1,10800);
    t.ui_cover=true; /* keep a staged fry from entering the birth UI */
    for(int second=0;second<=900;second++) {
        for(int i=0;i<2;i++) {
            t.fish[i].stage=STAGE_ADULT;
            t.fish[i].hunger=2;t.fish[i].stress=0;t.fish[i].energy=8;
        }
        if(second) progression_tick(&t,1);
        if(!trace_only && (second==0||second==899||second==900)) {
            fry_req_t r[FRY_REQ_MAX];bool staged;
            assert(progression_next_fry(&t,r,&staged)==3);
            assert(r[0].met==(second==900));assert(r[1].met);
            assert(fabsf(r[0].frac-second/900.0f)<.0001f);
            assert(staged==(second==900));
        }
    }
    cover(&t,VEG_NUB);progression_tick(&t,1);
    if(!trace_only) nursery_check(&t);
    /* A bought sword plant also counts as cover, as tank_nursery_bed specifies. */
    t.sd_unlocks|=SD_ITEM_PLANT;tank_veg_set(&t,3,VEG_NURSERY+.001f);
    if(!trace_only) nursery_check(&t);
    puts("PASS: nursery independent of pair/care; breeding still requires 15 active minutes");
    puts("PASS: growth, per-slot persistence and offline trace");
    return 0;
}
