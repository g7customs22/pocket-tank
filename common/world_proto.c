#include "world_proto.h"
#include "progression.h"
#include <math.h>
#include <string.h>
#include <limits.h>

static float clamp(float x,float lo,float hi){return x<lo?lo:x>hi?hi:x;}
static bool angles(const int16_t a[3],bool inv,float *roll,float *pitch){
 float x=a[0],y=a[1],z=a[2];
 if(fabsf(x)>=32000||fabsf(y)>=32000||fabsf(z)>=32000)return false;
 float n=sqrtf(x*x+y*y+z*z)/16384.f;if(n<.8f||n>1.2f)return false;
 if(inv){x=-x;y=-y;}
 *roll=atan2f(x,-y)*57.29578f;*pitch=atan2f(z,sqrtf(x*x+y*y))*57.29578f;return true;
}
void wp_camera_init(wp_camera_t *c){memset(c,0,sizeof *c);c->x=(TANK_W-WP_W)*.5f;c->y=TANK_H-WP_H;}
void wp_recenter(wp_camera_t *c,const int16_t a[3],bool inverted){float r,p;if(angles(a,inverted,&r,&p)){c->neutral_roll=r;c->neutral_pitch=p;c->ready=true;}}
static float dead(float a){return fabsf(a)<=2?0:(a>0?a-2:a+2);}
bool wp_camera_step(wp_camera_t *c,const int16_t a[3],bool inverted,int64_t stamp,int64_t now,float dt){
 float roll,pitch;
 if(stamp<=0||stamp>now||now-stamp>250000||!angles(a,inverted,&roll,&pitch)){c->rejected++;return false;}
 if(!c->ready)wp_recenter(c,a,inverted);
 c->last_sample=stamp;if(c->frozen)return true;
 float tx=clamp((TANK_W-WP_W)*.5f+dead(roll-c->neutral_roll)*(TANK_W-WP_W)/46.f,0,TANK_W-WP_W);
 float ty=clamp(TANK_H-WP_H-dead(pitch-c->neutral_pitch)*(TANK_H-WP_H)/23.f,0,TANK_H-WP_H);
 float alpha=1-expf(-clamp(dt,0,.1f)/.22f);c->x+=(tx-c->x)*alpha;c->y+=(ty-c->y)*alpha;return true;
}
void wp_origin(const wp_camera_t *c,int depth,float *x,float *y){
 static const float rate[]={.40f,1.f,1.25f};depth=depth<0?0:depth>2?2:depth;
 *x=roundf((TANK_W-WP_W)*.5f+(c->x-(TANK_W-WP_W)*.5f)*rate[depth]);
 *y=roundf((TANK_H-WP_H)+(c->y-(TANK_H-WP_H))*rate[depth]);
 /* Foreground overscan is clipped to existing world boundaries. */
 *x=clamp(*x,0,TANK_W-WP_W);*y=clamp(*y,0,TANK_H-WP_H);
}
void wp_map(const wp_camera_t *c,int d,float sx,float sy,float *x,float *y){wp_origin(c,d,x,y);*x+=sx;*y+=sy;}
void wp_project(const wp_camera_t *c,int d,float x,float y,float *sx,float *sy){float ox,oy;wp_origin(c,d,&ox,&oy);*sx=x-ox;*sy=y-oy;}
static bool allowed(int item){return item>=EXTRA_DECOR_FIRST&&item<=WP_AIRSTONE;}
bool wp_valid(const wp_saved_t *saved){
 for(int i=0;i<WP_COPIES;i++){const wp_copy_t *p=&saved->copy[i];if(p->id&&(p->id!=i+2||!allowed(p->item)||p->depth>2||!isfinite(p->x)||p->x<42||p->x>TANK_W-42))return false;}
 return true;
}
int wp_quantity(const tank_t *t,const wp_objects_t *o,int item){int n=item<SD_ITEM_COUNT&&item>=0&&!!(t->sd_unlocks&(1u<<item));for(int i=0;i<WP_COPIES;i++)n+=o->saved.copy[i].id&&o->saved.copy[i].item==item;return n;}
int wp_sources(const tank_t *t,const wp_objects_t *o,wp_source_t out[WP_SOURCES]){
 int n=0;out[n++]=(wp_source_t){0,t->bubble_x,t->bubble_y-74};
 if(t->sd_unlocks&(1u<<8))out[n++]=(wp_source_t){1,tank_decor_x(t,8),TANK_H-108};
 for(int i=0;i<WP_COPIES&&n<WP_SOURCES;i++){const wp_copy_t *p=&o->saved.copy[i];if(p->id&&(p->item==8||p->item==WP_AIRSTONE))out[n++]=(wp_source_t){p->id,p->x,TANK_H-108};}
 return n;
}
bool wp_move(wp_objects_t *o,int id,float x,int depth){
 if(!isfinite(x)||depth<0||depth>2)return false;
 for(int i=0;i<WP_COPIES;i++)if(o->saved.copy[i].id==id&&id>=2){o->saved.copy[i].x=clamp(x,42,TANK_W-42);o->saved.copy[i].depth=(uint8_t)depth;
  return true;}
 return false;
}
int wp_buy(tank_t *t,wp_objects_t *o,int item,float x,int depth){
 if(!allowed(item)||!isfinite(x)||depth<0||depth>2||(item<SD_ITEM_COUNT&&!tank_item_allowed(t->habitat,item)))return -1;
 wp_source_t sources[WP_SOURCES];if((item==8||item==WP_AIRSTONE)&&wp_sources(t,o,sources)>=WP_SOURCES)return -1;
 int price=item==WP_AIRSTONE?40:SD_ITEMS[item].price;if(t->sd_balance<price)return -1;
 for(int i=0;i<WP_COPIES;i++)if(!o->saved.copy[i].id){int id=i+2;o->saved.copy[i]=(wp_copy_t){(uint16_t)id,(uint8_t)item,(uint8_t)depth,clamp(x,42,TANK_W-42)};t->sd_balance-=price;return id;}
 return -1;
}
bool wp_sell(tank_t *t,wp_objects_t *o,int id){
 for(int i=0;i<WP_COPIES;i++)if(o->saved.copy[i].id==id&&id>=2){int item=o->saved.copy[i].item,refund=(item==WP_AIRSTONE?40:SD_ITEMS[item].price)*SD_SELL_PCT/100;
  t->sd_balance=t->sd_balance>INT_MAX-refund?INT_MAX:t->sd_balance+refund;memset(&o->saved.copy[i],0,sizeof(wp_copy_t));
  for(int f=0;f<N_FISH_MAX;f++)if(o->target[f]==id)o->assigned&=~(1u<<f);
  return true;}
 return false;
}
void wp_bind(tank_t *t,wp_objects_t *o){if(o!=&t->world)t->world=*o;}
void wp_bubble(tank_t *t,int fish,float *x,float *y,bool remember){
 *x=t->bubble_x;*y=t->bubble_y-74;if(fish<0||fish>=t->n_fish)return;
 wp_objects_t *bound_o=&t->world;
 wp_source_t sources[WP_SOURCES];int n=wp_sources(t,bound_o,sources),chosen=-1;
 if(bound_o->assigned&(1u<<fish))for(int i=0;i<n;i++)if(sources[i].id==bound_o->target[fish])chosen=i;
 if(chosen<0){float best=1e30f;for(int i=0;i<n;i++){float dx=sources[i].x-t->fish[fish].x,dy=sources[i].y-t->fish[fish].y;float d=dx*dx+dy*dy;if(d<best){best=d;chosen=i;}}}
 if(remember){bound_o->target[fish]=sources[chosen].id;bound_o->assigned|=1u<<fish;}
 *x=sources[chosen].x;*y=sources[chosen].y;
}
void wp_tick(tank_t *t,wp_objects_t *o,float dt,advisor_fn advise){wp_bind(t,o);for(int i=0;i<t->n_fish;i++)if(t->fish[i].goal.id!=GOAL_VISIT_BUBBLES)o->assigned&=~(1u<<i);tank_tick(t,dt,advise);if(o!=&t->world)*o=t->world;}
int wp_hit(const wp_camera_t *c,const wp_objects_t *o,float sx,float sy,int previous){
 int found[WP_COPIES],n=0;for(int d=2;d>=0;d--)for(int i=WP_COPIES-1;i>=0;i--){const wp_copy_t *p=&o->saved.copy[i];float x,y;if(!p->id||p->depth!=d)continue;wp_map(c,d,sx,sy,&x,&y);if(fabsf(x-p->x)<=38&&y>=TANK_H-110&&y<TANK_H)found[n++]=p->id;}
 if(!n)return -1;
 for(int i=0;i<n;i++)if(found[i]==previous)return found[(i+1)%n];
 return found[0];
}
void wp_wipe(tank_t *t,const wp_camera_t *c,float sx0,float sy0,float sx1,float sy1){float x0,y0,x1,y1;wp_map(c,2,sx0,sy0,&x0,&y0);wp_map(c,2,sx1,sy1,&x1,&y1);tank_proto_wipe(t,x0,y0,x1,y1);}
