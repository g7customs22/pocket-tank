#include "slot_system.h"
#include "world_proto.h"
#include "slot_journal.h"
#include "progression.h"
#include "esp_partition.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
static unsigned char flash[0x30000];
static const esp_partition_t partition={sizeof flash};
static int fail=-1,op;static bool partial;static int64_t wall=1800000000;
const esp_partition_t *esp_partition_find_first(int type,int sub,const char *name){assert(type==1&&sub==130&&!strcmp(name,"storage"));return &partition;}
int esp_partition_read(const esp_partition_t *p,uint32_t off,void *v,size_t n){assert(p==&partition&&off+n<=sizeof flash);memcpy(v,flash+off,n);return 0;}
int esp_partition_write(const esp_partition_t *p,uint32_t off,const void *v,size_t n){assert(p==&partition&&off>=0x10000&&off+n<=sizeof flash);bool stop=op++==fail;size_t take=stop?(partial?n/2:0):n;for(size_t i=0;i<take;i++)flash[off+i]&=((const unsigned char *)v)[i];return stop?-1:0;}
int esp_partition_erase_range(const esp_partition_t *p,uint32_t off,size_t n){assert(p==&partition&&off>=0x10000&&off+n<=sizeof flash);bool stop=op++==fail;memset(flash+off,255,stop?(partial?n/2:0):n);return stop?-1:0;}
bool persist_port_save(const void *v,size_t n){(void)v;(void)n;return slot_system_enabled()?slot_system_save():true;}
bool persist_port_load(void *v,size_t n,size_t *got){(void)v;(void)n;(void)got;return false;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return wall;}
const char *version_port_string(void){return "HOST";}

static unsigned char legacy[sizeof flash];
static void boot(tank_t *t){tank_init(t,123);assert(slot_system_boot(t)==1);}
int main(void){
 _Static_assert(sizeof(tank_t)==4072,"device ABI required");
 for(int version=4;version<=5;version++){
 FILE *f=fopen(version==5?"actual-v5.bin":"derived-v4.bin","rb");assert(f);assert(fread(legacy,1,sizeof legacy,f)==sizeof legacy);fclose(f);
 memcpy(flash,legacy,sizeof flash);wall=0;fail=-1;tank_t t;boot(&t);assert(slot_system_active()==1&&t.n_fish==2&&t.sd_balance==819&&t.sd_earned==1019&&!strcmp(t.fish[0].name,"ripple"));
 progression_context_t ctx;progression_context_capture(&ctx);float age=progression_age_s(&t,0);tank_t old=t;
 int id=wp_buy(&t,&t.world,8,555,1);assert(id>=2);int paid=t.sd_balance;assert(slot_system_save());assert(slot_system_switch(&t,2));assert(t.sd_balance==508&&!t.world.saved.copy[0].id);assert(slot_system_switch(&t,1));assert(t.sd_balance==paid&&t.world.saved.copy[0].id==id&&t.world.saved.copy[0].x==555);boot(&t);assert(t.world.saved.copy[0].id==id&&progression_age_s(&t,0)==age);progression_context_t after;progression_context_capture(&after);assert(after.s_breed_ready==ctx.s_breed_ready);
 for(int scenario=0;scenario<2;scenario++)for(int part=0;part<2;part++)for(int cut=0;cut<12;cut++){
  memcpy(flash,legacy,sizeof flash);fail=-1;boot(&t);partial=part!=0;wp_buy(&t,&t.world,8,555,1);op=0;fail=cut;bool ok=scenario?slot_system_switch(&t,2):slot_system_save();fail=-1;boot(&t);
  if(slot_system_active()==1){assert(t.n_fish==2&&!strcmp(t.fish[0].name,"ripple"));assert(t.sd_balance==(t.world.saved.copy[0].id?paid:819));assert(progression_age_s(&t,0)==age);if(ok&&!scenario)assert(t.world.saved.copy[0].id==id);}
  else assert(scenario&&slot_system_active()==2&&t.sd_balance==508);
  assert(!memcmp(flash,legacy,0x10000));
 }
 memcpy(flash,legacy,sizeof flash);fail=-1;boot(&t);assert(!slot_system_has_undo());assert(slot_system_reset(&t,123));assert(slot_system_has_undo());assert(slot_system_undo(&t));assert(t.sd_balance==819&&t.n_fish==old.n_fish);
 }
 puts("PASS actual device ABI v5 and derived v4 to v6; 819/508 credits, names/age/care preserved; duplicate placement across save/switch/reboot; reset/undo; 96 complete/partial power-loss cases; rescue unchanged");return 0;
}
