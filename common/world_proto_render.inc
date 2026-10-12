/* Native viewport-only proof renderer, reusing the production vector assets.
 * No full-world framebuffer or new glow/postprocess allocation. */
#include "world_proto.h"
static ctx_t wp_ctx(uint16_t *fb,const wp_camera_t *camera,int depth,float dim){float x,y;wp_origin(camera,depth,&x,&y);ctx_t c={fb,WP_W,dim,(int)x,(int)y,WP_W,WP_H};return c;}
static void wp_draw_item(ctx_t *c,const tank_t *t,int item,float x,float scale){
 if(x+100<c->ox||x-100>c->ox+c->w)return;
 if(item==WP_AIRSTONE){fill_ellipse(c,x,TANK_H-19,12,5,0x72a3ad,255);return;}
 if(item>=EXTRA_DECOR_FIRST)draw_extra(c,item,x,TANK_H-14,t->clock,scale,false);
 else if(item==2){draw_castle(c,(int)x,0,false);draw_castle(c,(int)x,1,false);}
 else if(item==3)draw_coral(c,(int)x,tank_coral_rgb(t),tank_coral_growth(t),false);
 else if(item==4)draw_cluster(c,(int)x,tank_cluster_scheme(t),tank_cluster_growth(t),false);
}
void render_world_proto(const tank_t *t,const wp_objects_t *o,const wp_camera_t *camera,uint16_t *fb){
 uint32_t *old_dirty=g_dirty;g_dirty=NULL;g_veg_mask_cx=-1;
 bool uv=t->uv.style==1&&!t->night;
 if(g_habitat!=t->habitat||g_uv_scene!=uv)g_veg_row_dim=g_castle_dim=g_coral_dim=g_cl_dim=-1;
 g_uv_scene=uv;g_uv_material=false;g_habitat=t->habitat;
 float dim=t->night?.45f:1.f;ctx_t far=wp_ctx(fb,camera,0,dim*.65f),mid=wp_ctx(fb,camera,1,dim),front=wp_ctx(fb,camera,2,dim);
 for(int y=0;y<WP_H;y++){uint16_t col=rgb565(water_rgb(y+mid.oy),dim);for(int x=0;x<WP_W;x++)fb[y*WP_W+x]=col;}
 /* Distant ridge and tall reed silhouettes make the far plane legible. */
 for(int k=0;k<12;k++){float x=60+k*120;fill_ellipse(&far,x,TANK_H-32,93,35,k%2?0x1b4146:0x25474c,255);
  for(int j=0;j<4;j++)fill_ellipse(&far,x-30+j*20,TANK_H-95-(k%3)*26,5,80+(j%2)*30,uv?0x25495c:0x285d55,255);}
 for(int d=0;d<3;d++){
  ctx_t *c=d==0?&far:d==1?&mid:&front;
  if(d==1){
   for(int x=0;x<TANK_W;x+=9)fill_ellipse(c,x,TANK_H-10,7,4,0x435243,255);
   for(int bed=0;bed<tank_veg_beds(t);bed++)draw_veg(c,t,bed,bed*13+1,-1,false);
  }
  for(int item=2;item<SD_ITEM_COUNT;item++)if(item!=5&&(t->sd_unlocks&(1u<<item))&&tank_decor_z(t,item)==d)wp_draw_item(c,t,item,tank_decor_x(t,item),1);
  for(int i=0;i<WP_COPIES;i++){const wp_copy_t *p=&o->saved.copy[i];if(p->id&&p->depth==d)wp_draw_item(c,t,p->item,p->x,d==2?1.12f:1.f);}
  if(d==1){
   wp_source_t src[WP_SOURCES];int n=wp_sources(t,o,src);
   for(int k=0;k<MAX_BUBBLE;k++){int i=k%n;float yy=TANK_H-23-fmodf(t->clock*(20+k%7)+k*47,TANK_H-30);float xx=src[i].x+sinf(t->clock+k)*6;
    fill_ellipse(c,xx,yy,1.5f+k%2,2,uv?0x958cee:0x8acbd7,110);}
   for(int i=0;i<t->n_fish;i++){const fish_t *f=&t->fish[i];if(f->x+65>=c->ox&&f->x-65<c->ox+WP_W&&f->y+40>=c->oy&&f->y-40<c->oy+WP_H)draw_fish(c,t,f);}
  }
 }
 draw_algae(&front,t);
 /* Subtle fixed glass border provides a stable reference during motion. */
 ctx_t screen={fb,WP_W,1,0,0,WP_W,WP_H};rect_fill(&screen,0,0,WP_W,1,0x254657);rect_fill(&screen,0,WP_H-1,WP_W,1,0x112c39);
 g_dirty=old_dirty;
}
