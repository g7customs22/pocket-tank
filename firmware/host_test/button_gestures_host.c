
#include "boot_light_349_v1.h"
#include <assert.h>
#include <stdio.h>
static boot349_button_t key;
static int poll(int ms,bool down){return boot349_poll(&key,(int64_t)ms*1000,down,false,false,false);}
static void reset(void){key=(boot349_button_t){0};assert(poll(0,false)==BOOT349_NONE);}
int main(void){
 reset();assert(poll(10,true)==0);assert(poll(100,false)==0);assert(poll(130,false)==0);assert(poll(449,false)==0);assert(poll(450,false)==BOOT349_PAGES);assert(poll(900,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);poll(200,true);poll(280,false);assert(poll(310,false)==BOOT349_STYLE);assert(poll(900,false)==0);
 reset();poll(10,true);assert(poll(709,true)==0);assert(poll(710,true)==BOOT349_LIGHT);assert(poll(2000,true)==0);poll(2100,false);assert(poll(2130,false)==0);assert(poll(3000,false)==0);
 reset();poll(10,true);poll(20,false);poll(50,false);assert(poll(1000,false)==0);
 reset();poll(10,true);poll(100,false);poll(115,true);poll(130,false);poll(160,false);assert(poll(480,false)==BOOT349_PAGES);
 reset();poll(10,true);assert(boot349_poll(&key,100000,true,true,false,false)==BOOT349_RESET);assert(poll(1000,true)==0);poll(1100,false);assert(poll(2000,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);assert(boot349_poll(&key,200000,false,false,true,false)==0);assert(poll(900,false)==0);
 key=(boot349_button_t){0};assert(poll(0,true)==0);assert(poll(2000,true)==0);poll(2100,false);assert(poll(3000,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);poll(200,true);assert(poll(900,true)==BOOT349_LIGHT);poll(1000,false);assert(poll(1030,false)==0);assert(poll(2000,false)==0);
 puts("PASS 350ms single latency, double style arbitration, long one-shot, bounce/minimum duration, reset chord, protected-page cancellation, boot-held key, second-press hold");
}
