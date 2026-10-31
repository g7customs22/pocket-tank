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
static bool legacy_mode; static unsigned char legacy[4096]; static size_t legacy_n;
bool persist_port_save(const void *v,size_t n){if(legacy_mode){assert(n<=sizeof legacy);memcpy(legacy,v,n);legacy_n=n;return true;}return slot_system_enabled()?slot_system_save():true;}
bool persist_port_load(void *v,size_t n,size_t *got){if(!legacy_mode||!legacy_n)return false;assert(n>=legacy_n);memcpy(v,legacy,legacy_n);*got=legacy_n;return true;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return wall;}
const char *version_port_string(void){return "HOST";}
int main(void){
 static tank_t t,paused,unchanged;memset(flash,255,sizeof flash);memset(flash,0x42,0x6000);
 tank_init(&t,19);assert(slot_system_boot(&t)==0);progression_fresh(&t);
 for(int i=2;i<5;i++)tank_make_fish(&t,i,i%tank_roster_count(),.5f,.5f,STAGE_ADULT);
 t.n_fish=5;strcpy(t.fish[0].name,"saved");t.sd_balance=1234;t.fish[0].hunger=2;
 progression_set_age(&t,0,10000);t.food[0].alive=true;t.food[0].x=220;t.food[0].y=100;
 progression_context_t before,after;progression_context_capture(&before);before.s_breed_state_version=1;before.s_breed_bad_cs=12345;before.s_breed_pair=2;before.s_breed_species=SPECIES_TIGER_BARB;before.s_breed_habitat=0;before.s_breed_ready=321;before.s_breed_cool=654;progression_context_restore(&before);unchanged=t;assert(slot_system_migrate(&t));assert(!memcmp(&unchanged,&t,sizeof t));assert(slot_system_present()==7);paused=t;float age=progression_age_s(&t,0);
 assert(slot_system_switch(&t,1));assert(t.n_fish==2&&t.habitat==1&&slot_system_active()==1);
 assert(t.fish[0].species==SPECIES_NEON&&t.fish[1].species==SPECIES_GUPPY);
 assert(t.sd_unlocks==(SD_ITEM_FERN|SD_ITEM_WOOD|SD_ITEM_STONES));
 assert(slot_system_switch(&t,2));assert(t.fish[0].species==SPECIES_CLOWN&&t.fish[1].species==SPECIES_GRAMMA);
 assert(t.sd_unlocks==(SD_ITEM_LIVEROCK|SD_ITEM_PLATE|SD_ITEM_ANEMONE));assert(slot_system_switch(&t,1));
 t.sd_balance=456;wall+=86400;
 assert(slot_system_switch(&t,0));assert(!memcmp(&paused,&t,sizeof t));assert(progression_age_s(&t,0)==age);progression_context_capture(&after);assert(after.s_breed_ready==321&&after.s_breed_cool==654);
 assert(after.s_breed_bad_cs==12345&&after.s_breed_pair==2&&after.s_breed_species==SPECIES_TIGER_BARB&&after.s_breed_state_version==1);
 puts("PASS: five-fish migration, names/balance/pellets/position preserved; inactive slot frozen across a day");
 wall+=7200;tank_init(&t,1);assert(slot_system_boot(&t)==1);
 progression_context_capture(&after);assert(after.s_breed_bad_cs==12345&&after.s_breed_pair==2&&after.s_breed_ready==321);
 assert(t.n_fish==5&&t.sd_balance==1234&&fabsf(t.fish[0].hunger-3.6f)<.01f);
 assert(fabsf(progression_age_s(&t,0)-(age+1800))<.01f);
 puts("PASS: selected tank alone receives two-hour offline hunger/age catchup");
 assert(slot_system_switch(&t,1));assert(t.sd_balance==456);
 unchanged=t;op=0;fail=0;assert(!slot_system_switch(&t,0));fail=-1;assert(!memcmp(&t,&unchanged,sizeof t));
 assert(slot_system_reset(&t,31));assert(t.n_fish==2&&t.sd_balance==0);
 assert(slot_system_has_undo());for(int i=0;i<4;i++)assert(slot_system_save());
 wall+=30;tank_init(&t,3);assert(slot_system_boot(&t)==1);assert(slot_system_has_undo());
 unchanged=t;op=0;fail=0;assert(!slot_system_undo(&t));fail=-1;assert(!memcmp(&unchanged,&t,sizeof t));
 assert(slot_system_undo(&t));assert(t.sd_balance==456);assert(slot_system_active()==1);
 t.sd_balance=789;assert(slot_system_reset(&t,44));assert(slot_system_undo(&t));assert(t.sd_balance==789);
 assert(slot_system_switch(&t,0));assert(t.n_fish==5&&t.sd_balance==1234);assert(!slot_system_has_undo());
 unchanged=t;op=0;fail=0;assert(!slot_system_reset(&t,12));fail=-1;assert(!memcmp(&t,&unchanged,sizeof t));
 for(unsigned i=0;i<0x6000;i++)assert(flash[i]==0x42);
 printf("PASS: save-failure switch rollback, per-slot reset, reset-failure rollback, rescue untouched; tank=%zu context=%zu bytes\n",sizeof(tank_t),sizeof(progression_context_t));
 puts("PASS: distinct seeded habitats, exact Standard migration, per-slot undo survives autosaves and reboot");
 progression_tick(&t,.1f);progression_sd_take_award();t.sd_balance=100;t.sd_earned=200;
 t.player_feedings++;progression_tick(&t,.1f);assert(t.sd_balance==104&&t.sd_earned==204);assert(slot_system_save());
 tank_init(&t,4);assert(slot_system_boot(&t)==1);assert(t.sd_balance==104&&t.sd_earned==204);
 progression_tick(&t,.1f);assert(t.sd_balance==104&&t.sd_earned==204);
 puts("PASS doubled credit persisted, balances unchanged on reboot, paid meal not paid twice");
 unchanged=t;int before_op=op;assert(slot_system_switch(&t,slot_system_active()));assert(op==before_op&&!memcmp(&t,&unchanged,sizeof t));
 puts("PASS active slot is a no-op: no flash writes and no tank mutation");

 const int64_t base=1800000000, jumps[]={0,7200,-7200,604800,604801,900000};
 for(int path=0;path<2;path++)for(int i=0;i<6;i++) {
  legacy_mode=path==1;wall=base;progression_set_age(&t,0,10000);
  if(legacy_mode)assert(progression_save(&t));else assert(slot_system_save());
  float before_clock=t.clock; wall=i==0?0:base+jumps[i];
  if(legacy_mode)progression_wake(&t,wall);else assert(slot_system_boot(&t)==1);
  float expected=(i==1||i==3)?(float)jumps[i]:0;
  assert(fabsf(progression_age_s(&t,0)-(10000+expected*.25f))<.1f);
  float phase=before_clock+expected;
  if(expected>0 && phase>=32768)phase=fmodf(phase,16384);
  assert(fabsf(t.clock-phase)<1); /* Animation phase wraps; credited age above does not. */
 }
 for(int path=0;path<2;path++) {
  legacy_mode=path==1;wall=0;progression_set_age(&t,0,10000);
  for(int boot=0;boot<3;boot++) {
   if(legacy_mode)assert(progression_save(&t));else assert(slot_system_save());
   wall=boot==2?base:0;
   if(legacy_mode)progression_wake(&t,wall);else assert(slot_system_boot(&t)==1);
   assert(progression_age_s(&t,0)==10000);
  }
 }
 puts("PASS both production loaders: unset, normal 2h, backwards/future save, exact cap, excessive/stale gap; repeated unknown boots and valid baseline recovery");
 return 0;
}
