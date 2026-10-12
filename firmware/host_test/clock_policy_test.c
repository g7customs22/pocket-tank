#include "clock_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 int v[6];uint8_t b[7]={0x59,0x59,0x23,0x29,4,0x02,0x24};
 assert(clock_rtc_decode(b,v));b[6]=0x25;assert(!clock_rtc_decode(b,v));
 b[6]=0x24;b[0]|=0x80;assert(!clock_rtc_decode(b,v));b[0]=0x6a;assert(!clock_rtc_decode(b,v));
 b[0]=0x59;b[5]=0;assert(!clock_rtc_decode(b,v));b[5]=0x13;assert(!clock_rtc_decode(b,v));
 assert(!clock_rtc_advancing(100,100));assert(!clock_rtc_advancing(100,99));
 assert(!clock_rtc_advancing(100,103));assert(clock_rtc_advancing(100,101));assert(clock_rtc_advancing(100,102));
 assert(clock_offline_seconds(INT64_MAX,1,604800)==0);
 puts("PASS RTC oscillator-stop, malformed BCD/calendar, leap day, stopped/backward/jumping tick, valid advancing tick and overflow boundary");return 0;}
