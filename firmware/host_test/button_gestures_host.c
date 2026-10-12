
#include "boot_light_349_v1.h"
#include <assert.h>
#include <stdio.h>
static boot349_button_t key;
static int poll(int ms,bool down){return boot349_poll(&key,(int64_t)ms*1000,down,false,false,false);}
static void reset(void){key=(boot349_button_t){0};assert(poll(0,false)==BOOT349_NONE);}
int main(void){
 reset();poll(10,true);poll(100,false);assert(poll(129,false)==0);assert(poll(130,false)==BOOT349_PAGES);assert(poll(450,false)==0);
 poll(500,true);poll(580,false);assert(poll(610,false)==BOOT349_PAGES); /* two ordinary short presses, no double action */
 reset();poll(10,true);assert(poll(709,true)==0);assert(poll(710,true)==BOOT349_LIGHT);assert(poll(2000,true)==0);poll(2100,false);assert(poll(2130,false)==0);
 reset();poll(10,true);poll(20,false);assert(poll(50,false)==0);assert(poll(1000,false)==0);
 reset();poll(10,true);poll(100,false);poll(115,true);poll(130,false);assert(poll(160,false)==BOOT349_PAGES);
 reset();poll(10,true);assert(boot349_poll(&key,100000,true,true,false,false)==BOOT349_RESET);assert(poll(1000,true)==0);poll(1100,false);assert(poll(1130,false)==0);
 reset();poll(10,true);assert(boot349_poll(&key,100000,true,false,true,false)==0);poll(120,false);assert(poll(150,false)==0);
 key=(boot349_button_t){0};assert(poll(0,true)==0);assert(poll(2000,true)==0);poll(2100,false);assert(poll(3000,false)==0);
 puts("PASS immediate debounced short release, no double gesture/delay, long one-shot/no short, bounce/minimum duration, reset chord, protected-page cancellation, boot-held key");
}
