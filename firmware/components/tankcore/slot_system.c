#include "slot_system.h"
#include "slot_journal.h"
#include "progression.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>
/* Storage+0..0x5fff belongs to NVS rescue. This separate arena is claimed
 * only if erased, or reopened with its own validated versioned journal. */
#define ARENA_BASE 0x10000u
#define SCHEMA 0x00010005u
#define LEGACY_SCHEMA 0x00010004u
typedef struct { tank_t tank; progression_context_t progression; int64_t saved_unix; } snapshot_t;
_Static_assert(sizeof(snapshot_t)<SJ_PAYLOAD_MAX,"snapshot exceeds bank");
#ifdef ESP_PLATFORM
_Static_assert(offsetof(tank_t,uv)==3976 && sizeof(tank_t)==3992 && sizeof(progression_context_t)==88 && sizeof(snapshot_t)==4088,
               "Bump slot schema and provide migration before changing persisted layouts");
#endif
/* Legacy prefix remains exact; a verified old record is upgraded in RAM only.
 * Journal writes use inactive banks/header-last and retain prior verified data. */
typedef struct { unsigned char tank[offsetof(tank_t,uv)]; progression_context_t progression; int64_t saved_unix; } legacy_snapshot_t;
static bool upgrade_snapshot(void *data,size_t old_len,size_t new_len){
    if(old_len!=sizeof(legacy_snapshot_t)||new_len!=sizeof(snapshot_t))return false;
    legacy_snapshot_t *old=data;snapshot_t *now=data;
    progression_context_t progression=old->progression;int64_t saved=old->saved_unix;
    memset((unsigned char *)data+offsetof(tank_t,uv),0,sizeof(snapshot_t)-offsetof(tank_t,uv));
    now->progression=progression;now->saved_unix=saved;return true;
}
static sj_t journal;
static const esp_partition_t *partition;
static tank_t *live;
static snapshot_t *buffer,*rollback;
static bool ready,blocked;
static bool rd(void *p,uint32_t off,void *v,size_t n){(void)p;return off+n<=SJ_BYTES&&esp_partition_read(partition,ARENA_BASE+off,v,n)==ESP_OK;}
static bool wr(void *p,uint32_t off,const void *v,size_t n){(void)p;return off+n<=SJ_BYTES&&esp_partition_write(partition,ARENA_BASE+off,v,n)==ESP_OK;}
static bool er(void *p,uint32_t off,size_t n){(void)p;return off+n<=SJ_BYTES&&esp_partition_erase_range(partition,ARENA_BASE+off,n)==ESP_OK;}
static void capture(snapshot_t *s){
    memset(s,0,sizeof *s);s->tank=*live;progression_context_capture(&s->progression);s->saved_unix=clock_port_now_unix();
    /* Interaction gestures aren't part of a paused aquarium. */
    s->tank.drag_active=s->tank.drag_has_prev=s->tank.slash_armed=s->tank.slash_engaged=s->tank.slash_cut=false;
    s->tank.hold_active=false;s->tank.hold_time=0;s->tank.ui_cover=false;s->tank.hold_light=false;
    s->tank.tap_count=0;
    for(int i=0;i<N_FISH_MAX;i++)s->tank.fish[i].model_name=NULL;
}
static bool valid(const snapshot_t *s){return s->tank.n_fish>=2&&s->tank.n_fish<=N_FISH_MAX&&s->tank.habitat<HABITAT_COUNT;}
static void restore(const snapshot_t *s){*live=s->tank;
    for(int i=0;i<live->n_fish;i++)live->fish[i].model_name=TRAINED_NAMES[i%N_TRAINED_NAMES];
    progression_context_restore(&s->progression);}
static void fresh(uint32_t seed,unsigned habitat){
    progression_context_t clean={0};
    clean.s_spawn_in=-1;clean.s_newborn=-1;clean.s_sd_prev_feedings=-1;
    progression_context_restore(&clean);memset(live,0,sizeof *live);tank_init(live,seed);progression_fresh(live);tank_habitat_defaults(live,(int)habitat);
}
bool slot_system_enabled(void){return ready;}
bool slot_system_blocked(void){return blocked;}
unsigned slot_system_active(void){return journal.meta.active;}
unsigned slot_system_present(void){return ready?journal.meta.present:0;}
int slot_system_boot(tank_t *tank){
    live=tank;blocked=true;ready=false;
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"storage");
    if(!partition||partition->size<ARENA_BASE+SJ_BYTES)return -1;
    buffer=heap_caps_calloc(1,sizeof *buffer,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    rollback=heap_caps_calloc(1,sizeof *rollback,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    void *scratch=heap_caps_malloc(SJ_BANK,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!buffer||!rollback||!scratch)return -1;
    if(!sj_mount_compat(&journal,(sj_io_t){0,rd,wr,er},SCHEMA,LEGACY_SCHEMA,sizeof(legacy_snapshot_t),upgrade_snapshot,scratch,SJ_BANK)){
        ESP_LOGE("slots","storage ownership/version/integrity check failed; no formatting, saves blocked");return -1;
    }
    if(!journal.meta.present){blocked=false;return 0;}
    if(!sj_load(&journal,journal.meta.active,buffer,sizeof *buffer)||!valid(buffer))return -1;
    restore(buffer);ready=true;blocked=false;
    int64_t now=clock_port_now_unix();
    /* Only selected tank advances through sleep/offline absence. Inactive
     * snapshots are never loaded here and never receive an elapsed interval. */
    if(now>buffer->saved_unix&&buffer->saved_unix>0){
        int64_t dt=now-buffer->saved_unix;if(dt>PROGRESSION_SLEEP_CAP_S)dt=PROGRESSION_SLEEP_CAP_S;
        progression_slept(live,(float)dt);
    }
    progression_woke(live);
    ESP_LOGI("slots","restored active tank %u; %u-byte snapshot",(unsigned)journal.meta.active+1,(unsigned)sizeof *buffer);
    return 1;
}
bool slot_system_migrate(tank_t *tank){
    if(blocked)return false;
    if(ready)return true;
    live=tank;capture(buffer);
    if(!sj_save(&journal,0,buffer,sizeof *buffer)||!sj_activate(&journal,0)){blocked=true;return false;}
    ready=true;capture(rollback);
    for(unsigned slot=1;slot<3;slot++){
        fresh((uint32_t)clock_port_now_unix()^(0x534c4f54u+slot),slot);capture(buffer);
        if(!sj_save(&journal,slot,buffer,sizeof *buffer)||!sj_register(&journal,slot)){
            restore(rollback);blocked=true;return false;
        }
    }
    restore(rollback);
    if(!sj_activate(&journal,0)){blocked=true;return false;}
    ESP_LOGI("slots","Standard preserved; Freshwater and Saltwater defaults created; original NVS retained");return true;
}
bool slot_system_save(void){
    if(!ready||blocked)return false;
    capture(buffer);
    return sj_save(&journal,journal.meta.active,buffer,sizeof *buffer);
}
bool slot_system_switch(tank_t *tank,unsigned slot){
    if(!ready||blocked||slot>=3)return false;
    if(slot==journal.meta.active)return true;
    live=tank;
    if(!slot_system_save())return false;
    capture(rollback);
    if(journal.meta.present&(1u<<slot)){
        if(!sj_load(&journal,slot,buffer,sizeof *buffer)||!valid(buffer))return false;
        restore(buffer); /* Deliberately NO progression_slept/progression_woke. */
    }else{
        fresh((uint32_t)clock_port_now_unix()^(0x534c4f54u+slot),slot);
    }
    /* Refresh activation timestamp before changing selected-slot metadata. */
    capture(buffer);
    if(!sj_save(&journal,slot,buffer,sizeof *buffer)){restore(rollback);return false;}
    if(!sj_activate(&journal,slot)){restore(rollback);blocked=true;return false;}
    ESP_LOGI("slots","tank %u active; other tanks frozen",slot+1);return true;
}
bool slot_system_reset(tank_t *tank,uint32_t seed){
    if(!ready||blocked)return false;
    live=tank;capture(rollback);
    if(!sj_undo_save(&journal,journal.meta.active,rollback,sizeof *rollback))return false;
    unsigned habitat=tank->habitat;fresh(seed,habitat);
    if(!slot_system_save()){restore(rollback);return false;}
    return true;
}

bool slot_system_has_undo(void){return ready&&!blocked&&sj_undo_load(&journal,journal.meta.active,buffer,sizeof *buffer)&&valid(buffer);}
int slot_system_fish_count(unsigned slot){
    if(!ready||blocked)return -1;
    if(slot==journal.meta.active)return live->n_fish;
    return sj_load(&journal,slot,buffer,sizeof *buffer)&&valid(buffer)?buffer->tank.n_fish:-1;
}
bool slot_system_undo(tank_t *tank){
    if(!ready||blocked)return false;
    live=tank;capture(rollback);
    if(!sj_undo_load(&journal,journal.meta.active,buffer,sizeof *buffer)||!valid(buffer))return false;
    restore(buffer);
    if(!slot_system_save()){restore(rollback);return false;}
    /* Backup deliberately persists until the next confirmed reset. */
    return true;
}
