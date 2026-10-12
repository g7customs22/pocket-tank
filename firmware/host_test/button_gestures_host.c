#include "boot_light_349_v1.h"
#include "button_deployed_74d3237.h"
#include <assert.h>
#include <stdio.h>
static boot349_button_t key;
static int poll(int ms,bool down){return boot349_poll(&key,(int64_t)ms*1000,down,false,false,false);}
static void reset(void){key=(boot349_button_t){0};assert(poll(0,false)==0);}
static void unit(void){
 reset();poll(10,true);poll(100,false);poll(130,false);assert(poll(599,false)==0);assert(poll(600,false)==BOOT349_PAGES);assert(poll(900,false)==0);
 reset();poll(10,true);poll(100,false);assert(poll(140,true)==0);poll(220,false);assert(poll(250,false)==BOOT349_STYLE);
 reset();poll(10,true);poll(100,false);poll(130,false);assert(poll(600,true)==0);poll(1100,false);assert(poll(1130,false)==BOOT349_STYLE);assert(poll(2000,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);assert(poll(601,true)==BOOT349_PAGES);poll(700,false);poll(730,false);assert(poll(1200,false)==BOOT349_PAGES);
 reset();poll(10,true);assert(poll(709,true)==0);assert(poll(710,true)==BOOT349_LIGHT);assert(poll(2000,true)==0);poll(2100,false);assert(poll(2130,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);poll(590,true);assert(poll(1289,true)==0);assert(poll(1290,true)==BOOT349_LIGHT);poll(2000,false);assert(poll(2030,false)==0);
 reset();poll(10,true);poll(59,false);poll(89,false);assert(poll(1000,false)==0);
 reset();poll(10,true);poll(60,false);poll(90,false);assert(poll(560,false)==BOOT349_PAGES);
 reset();poll(10,true);poll(100,false);poll(115,true);poll(130,false);poll(160,false);assert(poll(630,false)==BOOT349_PAGES);
 reset();poll(10,true);assert(boot349_poll(&key,100000,true,true,false,false)==BOOT349_RESET);assert(poll(1000,true)==0);poll(1100,false);assert(poll(2000,false)==0);
 reset();poll(10,true);poll(100,false);poll(130,false);assert(boot349_poll(&key,200000,false,false,true,false)==0);assert(poll(900,false)==0);
 key=(boot349_button_t){0};assert(poll(0,true)==0);assert(poll(2000,true)==0);poll(2100,false);assert(poll(3000,false)==0);
 reset();poll(10,true);poll(100,false);poll(140,true);poll(230,false);assert(poll(270,true)==BOOT349_STYLE);poll(360,false);poll(390,false);assert(poll(860,false)==BOOT349_PAGES);
 reset();poll(10,true);poll(710,false);assert(poll(740,false)==BOOT349_LIGHT);
 boot349_capture_t c={0};for(int i=1;i<100;i++)boot349_capture(&c,i,(i&1)!=0);assert(c.overflow&&c.count==BOOT349_EDGE_CAP);
 reset();poll(10,true);poll(100,false);poll(129,true);poll(220,false);poll(250,false);assert(poll(720,false)==BOOT349_PAGES);
 reset();poll(10,true);poll(100,false);poll(130,false);poll(590,true);poll(1289,false);assert(poll(1319,false)==BOOT349_STYLE);
 reset();poll(10,true);poll(100,false);poll(130,false);assert(boot349_poll(&key,200000,false,false,false,true)==0);assert(poll(900,false)==0);
 puts("PASS 18 boundary/arbitration/bounce/protection/boot-held/triple/overflow cases");
}
static bool physical(int t,int start,int width,int gap){return (t>=start&&t<start+width)||(t>=start+width+gap&&t<start+2*width+gap);}
/* Same edge capture and recognizer as firmware; delayed tank dispatch. */
static int simulate(bool old,int width,int gap,int phase,int frame){
 boot349_button_t k={.armed=true};old_boot349_button_t o={.armed=true};boot349_capture_t c={0};bool replay=false;int styles=0,pages=0,lights=0;
 for(int t=0;t<=2500;t++){
  bool down=physical(t,phase+10,width,gap);
  if(!old&&t%5==0)boot349_capture(&c,(int64_t)t*1000,down);
  if(t%frame)continue;
  if(old){int a=old_boot349_poll(&o,(int64_t)t*1000,down,false,false,false);styles+=a==OLD_BOOT349_STYLE;pages+=a==OLD_BOOT349_PAGES;lights+=a==OLD_BOOT349_LIGHT;}
  else {
   assert(!c.overflow);
   for(unsigned i=0;i<c.count;i++){replay=c.edges[i].down;int a=boot349_poll(&k,c.edges[i].at,replay,false,false,false);styles+=a==BOOT349_STYLE;pages+=a==BOOT349_PAGES;lights+=a==BOOT349_LIGHT;}
   c.count=0;int a=boot349_poll(&k,(int64_t)t*1000,replay,false,false,false);styles+=a==BOOT349_STYLE;pages+=a==BOOT349_PAGES;lights+=a==BOOT349_LIGHT;
  }
 }
 return styles==1&&pages==0&&lights==0;
}
int main(int argc,char **argv){
 unit();FILE *f=NULL;assert(fopen_s(&f,argc>1?argv[1]:"button-acceptance.csv","w")==0);fputs("press_ms,release_gap_ms,deployed_success_of_150,hotfix_success_of_150\n",f);
 int total=0,oldpass=0,newpass=0;
 for(int width=50;width<=300;width+=10)for(int gap=30;gap<=500;gap+=10){int a=0,b=0;for(int phase=0;phase<150;phase++){a+=simulate(true,width,gap,phase,150);b+=simulate(false,width,gap,phase,150);}fprintf(f,"%d,%d,%d,%d\n",width,gap,a,b);total+=150;oldpass+=a;newpass+=b;assert(b==150);}
 fclose(f);printf("PASS sweep: deployed %d/%d vs hotfix %d/%d (50-300ms widths,30-500ms gaps,150 phases;150ms dispatch)\n",oldpass,total,newpass,total);
 for(int width=60;width<=300;width+=60)for(int gap=40;gap<=490;gap+=50)for(int phase=0;phase<400;phase++)assert(simulate(false,width,gap,phase,400));
 puts("PASS 20000 additional delayed-dispatch trials at400ms; no duplicate/menu/power events");
 for(int phase=0;phase<154;phase++)for(int w=51;w<300;w+=17)for(int g=31;g<499;g+=19)assert(simulate(false,w,g,phase,154));
 puts("PASS non-grid widths/gaps and154ms frame cadence (5ms sample quantization)");
 return 0;
}
