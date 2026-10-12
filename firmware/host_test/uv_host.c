
#define main old_care_main
#include "care_ui_host.c"
#undef main
#include "boot_light_349_v1.h"
#include "ui_349_v1.h"
#include <time.h>
static void ppm(const char *name,const uint16_t *fb,int w,int h,int stride){char p[160];snprintf(p,sizeof p,"outputs/uv-review/%s.ppm",name);FILE *f=fopen(p,"wb");assert(f);fprintf(f,"P6\n%d %d\n255\n",w,h);for(int y=0;y<h;y++)for(int x=0;x<w;x++){uint16_t c=fb[y*stride+x];unsigned char v[3]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(v,1,3,f);}fclose(f);}
int main(void){
 setvbuf(stdout,NULL,_IONBF,0);tank_t t;ready(&t);
 assert(SD_ITEM_SHELLS==(1u<<22)&&SD_ITEM_NEON_FERN==(1u<<23)&&SD_ITEM_NEON_GRASS==(1u<<24));
 assert(SD_ITEMS[23].price==SD_ITEMS[11].price&&SD_ITEMS[24].price==SD_ITEMS[9].price);
 t.sd_balance=54;assert(!progression_buy(&t,23));t.sd_balance=100;assert(progression_buy(&t,23)&&t.sd_balance==45);assert(!progression_buy(&t,23));assert(progression_buy(&t,24)&&t.sd_balance==0);
 tank_decor_set(&t,23,400,DECOR_Z_FRONT);tank_decor_set(&t,24,900,DECOR_Z_BACK);assert(tank_decor_x(&t,23)==400&&tank_decor_x(&t,24)==900);
 t.night=true;t.light_manual_off=true;boot349_toggle_style(&t);assert(t.uv.style==1&&t.light_manual_off&&t.night);boot349_toggle_light(&t);assert(!t.light_manual_off&&t.uv.style==1);
 tank_t other=t;other.uv.style=0;for(int i=0;i<100;i++){tank_tick(&t,.1f,advisor_rules);tank_tick(&other,.1f,advisor_rules);}tank_uv_t keep=t.uv;t.uv=other.uv;assert(!memcmp(&t,&other,sizeof t));t.uv=keep;
 size_t n=TANK_W*TANK_H;uint16_t *a=calloc(n+2,2),*b=calloc(n+2,2),*off=calloc(n+2,2);assert(a&&b&&off);a[0]=b[0]=off[0]=0x1234;a[n+1]=b[n+1]=off[n+1]=0xabcd;
 clock_t start=clock();int differs=0;
 for(int item=0;item<SD_ITEM_COUNT;item++){
  ready(&t);t.sd_unlocks=1u<<item;t.night=false;t.clock=2;t.uv.style=0;render_tank(&t,a+1,TANK_W);t.uv.style=1;render_tank(&t,b+1,TANK_W);
  assert(memcmp(a+1,b+1,n*2));differs++;
  t.night=true;t.uv.style=0;render_tank(&t,a+1,TANK_W);t.uv.style=1;render_tank(&t,off+1,TANK_W);assert(!memcmp(a+1,off+1,n*2));
  assert(a[0]==0x1234&&a[n+1]==0xabcd&&b[0]==0x1234&&b[n+1]==0xabcd&&off[0]==0x1234&&off[n+1]==0xabcd);
 }
 printf("PASS %d catalog items white/UV/off, framebuffer guards; off identical; physiology identical; %.2fs host render sweep\n",differs,(double)(clock()-start)/CLOCKS_PER_SEC);

 uint16_t *scene=calloc(n+2,2);uint32_t *dirty=calloc(RENDER_DIRTY_WORDS+2,4);uint8_t *vig=calloc(n+2,1);assert(scene&&dirty&&vig);
 scene[0]=scene[n+1]=0x99aa;dirty[0]=dirty[RENDER_DIRTY_WORDS+1]=0x1234abcd;vig[0]=vig[n+1]=0x5a;
 render_set_scene_cache(scene+1);render_set_dirty_mask(dirty+1);render_set_vignette_cache(vig+1);
 for(int item=0;item<SD_ITEM_COUNT;item++){
  ready(&t);t.sd_unlocks|=1u<<item;t.clock=2;
  for(int mode=0;mode<4;mode++){t.uv.style=mode&1;t.night=mode>=2;render_tank(&t,a+1,TANK_W);render_tank(&t,b+1,TANK_W);if(memcmp(a+1,b+1,n*2)){int diff=0;for(size_t q=0;q<n;q++)if(a[q+1]!=b[q+1]){if(diff<3)printf("DIFF item=%d mode=%d x=%zu y=%zu %04x %04x\n",item,mode,q%TANK_W,q/TANK_W,a[q+1],b[q+1]);diff++;}printf("DIFF count=%d\n",diff);assert(0);}}
 }
 assert(scene[0]==0x99aa&&scene[n+1]==0x99aa&&dirty[0]==0x1234abcd&&dirty[RENDER_DIRTY_WORDS+1]==0x1234abcd&&vig[0]==0x5a&&vig[n+1]==0x5a);
 render_set_scene_cache(NULL);render_set_dirty_mask(NULL);render_set_vignette_cache(NULL);free(scene);free(dirty);free(vig);
 puts("PASS all catalog materials in cached white/UV/off frames, style cache invalidation and frame/scene/dirty/vignette guards");
 for(int page=0;page<5;page++){
  uint16_t sheet[640*350]={0};ctx_t c={sheet,640,1,0,0,640,350};
  for(int row=0;row<5;row++){int item=page*5+row;int y=row*70;draw_text(&c,8,y+12,2,0xd7efef,SD_ITEMS[item].name);draw_text(&c,210,y+3,1,0xffffff,"WHITE");draw_text(&c,360,y+3,1,0xc1a0ff,"UV");material_preview(sheet,640,item,200,y+14,2,false);material_preview(sheet,640,item,350,y+14,2,true);}
  char name[30];snprintf(name,sizeof name,"catalog-%d",page+1);ppm(name,sheet,640,350,640);
 }
 uint16_t native[640*172];ui349_state_t u={0};u.controls_hidden=true;
 for(int h=0;h<3;h++){
  ready(&t);tank_habitat_defaults(&t,h);t.sd_unlocks|=SD_ITEM_PLANT|SD_ITEM_NEON_FERN|SD_ITEM_NEON_GRASS|SD_ITEM_CASTLE;
  for(int mode=0;mode<3;mode++){t.night=mode==2;t.uv.style=mode==1;render_tank(&t,a+1,TANK_W);ui349_render(&u,&t,a+1,native);char name[40];snprintf(name,sizeof name,"habitat-%d-mode-%d",h,mode);ppm(name,native,640,172,640);}
 }
 ready(&t);t.sd_balance=100;g_shp_modal=23;g_shp_earn=false;render_shop(&t,a+1,TANK_W);ppm("shop-neon-full",a+1,TANK_UI_W,TANK_H,TANK_W);u.controls_hidden=false;ui349_configure(&u,(ui349_view_t){2300,"SHOP",true,48});ui349_render(&u,&t,a+1,native);ppm("shop-neon-preview",native,640,172,640);
 /* Material thumbnails contain changed pixels, independent of purple water. */
 for(int item=0;item<SD_ITEM_COUNT;item++)if(tank_decor_placeable(item)){
  memset(a+1,0,n*2);memset(b+1,0,n*2);material_preview(a+1,TANK_W,item,0,0,2,false);material_preview(b+1,TANK_W,item,0,0,2,true);int changed=0;
  for(int y=0;y<54;y++)for(int x=0;x<64;x++){uint16_t day=a[1+y*TANK_W+x],uv=b[1+y*TANK_W+x];if(day!=rgb565(0x092028,1)&&uv!=rgb565(0x100b27,1)&&day!=uv)changed++;}assert(changed>5);
 }
 puts("PASS every ornamental thumbnail changes material color, purchases/prices/ownership/placement, style while off; visual sheets generated");
 free(a);free(b);free(off);return 0;
}
