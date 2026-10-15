#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
static struct tm *compat_gmtime(const time_t *t,struct tm *out){return gmtime_s(out,t)?NULL:out;}
#define gmtime_r compat_gmtime
#include "../components/tankcore/rtc_port_pcf85063.c"
static uint8_t regs[11];static int writes,saves,fail_write,fail_save;static time_t sysnow;static int64_t stamps[3];static bool stopped;
int i2c_master_bus_add_device(void*b,const i2c_device_config_t*c,void**d){*d=(void*)1;return 0;}
int i2c_master_transmit_receive(void*d,const uint8_t*r,size_t n,uint8_t*out,size_t z,int ms){memcpy(out,regs+*r,z);return 0;}
int i2c_master_transmit(void*d,const uint8_t*w,size_t n,int ms){writes++;if(writes==fail_write){regs[4]=0x80;return -1;}memcpy(regs+*w,w+1,n-1);return 0;}
int gettimeofday(struct timeval*t,void*z){t->tv_sec=sysnow;t->tv_usec=0;return 0;}
int settimeofday(const struct timeval*t,void*z){sysnow=t->tv_sec;return 0;}
bool slot_system_save(void){++saves;if(saves==fail_save)return false;stamps[saves-1]=s_trusted?sysnow:0;return true;}
void vTaskDelay(int ms){if(!stopped){int v=(regs[4]>>4)*10+(regs[4]&15)+1;regs[4]=(v/10)*16+v%10;}}
static void fresh(void){memset(regs,0,sizeof regs);regs[4]=0x80;regs[7]=1;regs[9]=1;regs[10]=0x26;s_dev=(void*)1;s_trusted=false;writes=saves=fail_write=fail_save=0;sysnow=0;stopped=false;memset(stamps,0,sizeof stamps);}
int main(void){
 int v[]={2024,2,29,12,0,0};int64_t epoch;assert(clock_edit_epoch(v,&epoch));assert(epoch==1709208000);
 v[0]=2025;assert(!clock_edit_epoch(v,&epoch));v[0]=2100;assert(!clock_edit_epoch(v,&epoch));v[0]=2026;v[1]=4;v[2]=31;assert(!clock_edit_epoch(v,&epoch));
 v[2]=30;v[3]=24;assert(!clock_edit_epoch(v,&epoch));v[3]=12;v[4]=60;assert(!clock_edit_epoch(v,&epoch));
 for(int direction=-1;direction<=1;direction+=2){fresh();sysnow=1800000000;s_trusted=true;time_t target=sysnow+direction*86400;assert(rtc_port_set_utc(target));assert(saves==2&&stamps[0]==0&&stamps[1]==target+1);assert(sysnow==target+1&&s_trusted);assert(clock_offline_seconds(sysnow,stamps[0],604800)==0);assert(clock_offline_seconds(sysnow,stamps[1],604800)==0);time_t got;assert(rtc_read(&got)&&got==sysnow);s_trusted=false;assert(rtc_port_init((void*)1)&&s_trusted);}
 fresh();fail_write=1;uint8_t original[11];memcpy(original,regs,11);assert(!rtc_port_set_utc(1800000000));assert(!memcmp(regs,original,11)&&!s_trusted&&stamps[0]==0);
 fresh();fail_save=1;assert(!rtc_port_set_utc(1800000000)&&writes==0);
 fresh();fail_save=2;assert(!rtc_port_set_utc(1800000000)&&!s_trusted&&sysnow==1800000001&&stamps[0]==0);
 fresh();stopped=true;memcpy(original,regs,11);assert(!rtc_port_set_utc(1800000000)&&!memcmp(regs,original,11)&&!s_trusted);
 fresh();regs[0]=32;assert(!rtc_port_set_utc(1800000000)&&writes==0&&saves==0);
 fresh();assert(!rtc_port_set_utc(1)&&writes==0);puts("PASS production RTC forward/backward sync, zero baseline, restart, invalid dates, stopped clock, write rollback and save failures");return 0;
}

