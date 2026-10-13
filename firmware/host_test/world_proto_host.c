#include "world_proto.h"
#include "progression.h"
#include "advisor.h"
#include "llm/advisor_core.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
bool persist_port_save(const void *p,size_t n){(void)p;(void)n;return true;}
bool persist_port_load(void *p,size_t n,size_t *got){(void)p;(void)n;*got=0;return false;}
bool persist_port_erase(void){return true;}
int64_t clock_port_now_unix(void){return 1800000000;}
const char *version_port_string(void){return "WORLD_HOST_PROTOTYPE";}
static void sample(float roll,float pitch,int16_t a[3]){float r=roll*.017453293f,p=pitch*.017453293f;a[0]=(int16_t)(16384*sinf(r)*cosf(p));a[1]=(int16_t)(-16384*cosf(r)*cosf(p));a[2]=(int16_t)(16384*sinf(p));}
static void ready(tank_t *t){tank_init(t,23);progression_fresh(t);progression_setup_done(t);t->light_auto=false;t->light_manual_off=false;t->sd_balance=10000;for(int i=0;i<t->n_fish;i++){progression_set_age(t,i,40000);t->fish[i].hunger=2;t->fish[i].energy=8;t->fish[i].stress=0;}}
static goal_t play(const tank_t *t,int i,bool req){(void)t;(void)i;(void)req;return (goal_t){GOAL_VISIT_BUBBLES,3,1,GOAL_COUNT};}
static void ppm(int frame,uint16_t *fb){char name[120];snprintf(name,sizeof name,"../outputs/world-prototype/frame-%03d.ppm",frame);FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n640 172\n255\n");for(int i=0;i<WP_W*WP_H;i++){uint16_t c=fb[i];unsigned char p[3]={(c>>11)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(p,1,3,f);}fclose(f);}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);_Static_assert(sizeof(wp_saved_t)==64,"fixed extension payload");
 wp_camera_t cam;wp_camera_init(&cam);int16_t a[3]={429,-16189,637};wp_recenter(&cam,a,false);
 for(int i=0;i<90;i++){a[0]=429+i%20-10;assert(wp_camera_step(&cam,a,false,100000+i*33000,100000+i*33000,1/30.f));}assert(cam.x==32&&cam.y==12);
 wp_camera_init(&cam);sample(0,0,a);wp_recenter(&cam,a,false);
 for(int i=0;i<150;i++){sample(15,0,a);wp_camera_step(&cam,a,false,100000+i*33000,100000+i*33000,1/30.f);}assert(cam.x<1 && cam.y==12);
 for(int i=0;i<150;i++){sample(-15,15,a);wp_camera_step(&cam,a,false,100000+i*33000,100000+i*33000,1/30.f);}assert(cam.x>63&&cam.y>23);
 for(int i=0;i<150;i++){sample(15,-15,a);wp_camera_step(&cam,a,false,100000+i*33000,100000+i*33000,1/30.f);}assert(cam.x<1&&cam.y<1);
 cam.frozen=true;float old=cam.x;sample(15,0,a);wp_camera_step(&cam,a,false,100000,100000,.1f);assert(cam.x==old);cam.frozen=false;
 assert(!wp_camera_step(&cam,a,false,100000,400001,.1f));a[0]=32767;assert(!wp_camera_step(&cam,a,false,400000,400000,.1f));
 for(int d=0;d<3;d++)for(int pos=0;pos<3;pos++){cam.x=pos*32;cam.y=pos*12;for(int y=0;y<WP_H;y++)for(int x=0;x<WP_W;x++){float wx,wy,sx,sy;wp_map(&cam,d,(float)x,(float)y,&wx,&wy);wp_project(&cam,d,wx,wy,&sx,&sy);assert(sx==x&&sy==y);}}
 puts("PASS recorded neutral/jitter, smoothing/end stops, stale/railed rejection, frozen gesture and all 990720 layer/camera pixel roundtrips");
 tank_t t;ready(&t);memset(&t.world,0,sizeof t.world);t.sd_unlocks|=1u<<7;int skull1=wp_buy(&t,&t.world,7,520,2),skull2=wp_buy(&t,&t.world,7,520,2);assert(skull1>=2&&skull2>=2&&wp_quantity(&t,&t.world,7)==3);
 wp_camera_init(&cam);float sx,sy;wp_project(&cam,2,520,TANK_H-30,&sx,&sy);int picked=wp_hit(&cam,&t.world,sx,sy,-1);assert(picked==skull2&&wp_hit(&cam,&t.world,sx,sy,picked)==skull1);assert(wp_move(&t.world,skull2,950,1));
 assert(wp_sell(&t,&t.world,skull1)&&wp_quantity(&t,&t.world,7)==2);wp_saved_t saved=t.world.saved;wp_objects_t restored={0};restored.saved=saved;assert(!memcmp(&restored.saved,&t.world.saved,sizeof saved));
 int c1=wp_buy(&t,&t.world,8,200,1),c2=wp_buy(&t,&t.world,8,1100,1),air=wp_buy(&t,&t.world,WP_AIRSTONE,650,1);assert(c1>=2&&c2>=2&&air>=2);int money=t.sd_balance;assert(wp_buy(&t,&t.world,8,700,1)<0&&t.sd_balance==money);
 t.fish[0].x=210;t.fish[0].y=260;t.fish[1].x=1090;t.fish[1].y=260;for(int i=0;i<2;i++)t.fish[i].goal.id=GOAL_VISIT_BUBBLES;wp_bind(&t,&t.world);float bx,by;wp_bubble(&t,0,&bx,&by,true);assert(bx==200);wp_bubble(&t,1,&bx,&by,true);assert(bx==1100);
 t.fish[0].x=1090;wp_bubble(&t,0,&bx,&by,true);assert(bx==200);assert(wp_sell(&t,&t.world,c1));wp_bubble(&t,0,&bx,&by,true);assert(bx!=200);
 t.fish[0].x=1100;t.fish[0].y=260;char encoding[640];advisor_core_encode(&t,0,encoding,sizeof encoding);assert(strstr(encoding,"bubble near"));tank_t copied=t;advisor_core_encode(&copied,0,encoding,sizeof encoding);assert(strstr(encoding,"bubble near"));
 wp_camera_init(&cam);cam.x=64;float offx=t.fish[1].x;t.fish[1].x=20;t.fish[1].goal.id=GOAL_VISIT_BUBBLES;float age=progression_age_s(&t,1);for(int i=0;i<120;i++){wp_tick(&t,&t.world,.05f,play);progression_tick(&t,.05f);}assert(progression_age_s(&t,1)>age+5&&t.fish[1].x!=20);(void)offx;
 puts("PASS duplicate purchases/quantities/independent moves/overlap cycling/single sale, four-source cap, stable per-fish choice and removal retarget, matching advisor sighting, offscreen simulation");
 ready(&t);memset(t.algae,255,sizeof t.algae);memset(t.veg_h,0,sizeof t.veg_h);wp_camera_init(&cam);cam.x=48;cam.y=12;float wx,wy;wp_map(&cam,2,200,60,&wx,&wy);int cell=(int)(wy/ALGAE_CELL)*ALGAE_COLS+(int)(wx/ALGAE_CELL_X);wp_wipe(&t,&cam,170,60,235,60);assert(t.algae[cell]<255);assert(t.algae[0]==255);
 puts("PASS visible foreground glass cleaning updates matching world cell, distant cell unchanged");
 ready(&t);memset(&t.world,0,sizeof t.world);t.sd_unlocks=SD_ITEM_PLANT|SD_ITEM_CASTLE|SD_ITEM_FERN|SD_ITEM_WOOD|SD_ITEM_STONES;t.uv.style=1;t.night=false;t.plant_x=780;t.castle_x=580;t.castle_z=DECOR_Z_BACK;
 tank_set_species(&t,0,SPECIES_TIGER_BARB);tank_set_species(&t,1,SPECIES_NEON);t.fish[0].x=550;t.fish[0].y=240;t.fish[1].x=1120;t.fish[1].y=235;
 wp_buy(&t,&t.world,8,380,1);wp_buy(&t,&t.world,8,1040,1);wp_buy(&t,&t.world,7,450,2);wp_buy(&t,&t.world,7,920,2);wp_buy(&t,&t.world,23,760,2);wp_buy(&t,&t.world,24,1180,2);wp_buy(&t,&t.world,19,620,0);wp_buy(&t,&t.world,WP_AIRSTONE,690,1);
 wp_camera_init(&cam);sample(0,0,a);wp_recenter(&cam,a,false);size_t n=WP_W*WP_H;uint16_t *buf=calloc(n+2,2);assert(buf);buf[0]=0x1234;buf[n+1]=0xabcd;
 clock_t begin=clock();FILE *trace=fopen("../outputs/world-prototype/camera.csv","w");assert(trace);fprintf(trace,"frame,roll,pitch,camera_x,camera_y,far_x,mid_x,front_x\n");
 for(int f=0;f<90;f++){float r=15*sinf(f*6.2831853f/90),p=12*fmaxf(0,sinf(f*6.2831853f/90));sample(r,p,a);
  for(int sub=0;sub<2;sub++){wp_camera_step(&cam,a,false,100000+(f*2+sub)*33333,100000+(f*2+sub)*33333,1/30.f);wp_tick(&t,&t.world,1/30.f,advisor_rules);progression_tick(&t,1/30.f);}
  render_world_proto(&t,&t.world,&cam,buf+1);assert(buf[0]==0x1234&&buf[n+1]==0xabcd);ppm(f,buf+1);float farx,y,midx,frontx;wp_origin(&cam,0,&farx,&y);wp_origin(&cam,1,&midx,&y);wp_origin(&cam,2,&frontx,&y);fprintf(trace,"%d,%.3f,%.3f,%.2f,%.2f,%.0f,%.0f,%.0f\n",f,r,p,cam.x,cam.y,farx,midx,frontx);
 }
 fclose(trace);printf("PASS 90 native viewport frames, framebuffer guards, eight copies / four sources; total render+physics+PPM CPU %.3fs (HOST ONLY)\n",(double)(clock()-begin)/CLOCKS_PER_SEC);
 printf("Memory: native RGB565 %zu bytes; persistent extension %zu; camera %zu; objects %zu; existing world tank %zu bytes\n",n*2,sizeof(wp_saved_t),sizeof cam,sizeof t.world,sizeof t);free(buf);return 0;
}
