#include "ui_349_v1.h"
#include "viewport_349_v1.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tap(ui349_state_t *s, int x, int y) {
    bool down; int tx,ty;
    assert(ui349_input(s,true,x,y,&down,&tx,&ty)==UI349_CONSUMED);
    return ui349_input(s,false,0,0,&down,&tx,&ty);
}
int main(void) {
    assert(v1_backlight_drive(0)==0);
    for(unsigned v=1;v<=255;v++) {
        assert(v1_backlight_drive(v)>=153 && v1_backlight_drive(v)<=255);
        assert(v1_backlight_drive(v)>=v1_backlight_drive(v-1));
    }
    assert(v1_backlight_drive(255)==255);
    for(unsigned pixel=0;pixel<65536;pixel++) {
        uint16_t full=v1_dim_pixel((uint16_t)pixel,v1_image_gain(100));
        uint16_t mid=v1_dim_pixel((uint16_t)pixel,v1_image_gain(60));
        uint16_t low=v1_dim_pixel((uint16_t)pixel,v1_image_gain(30));
        assert(full==pixel);
        for(unsigned shift=0;shift<=11;shift+=(shift==0?5:6)) {
            unsigned mask=shift==5?63:31;
            assert(((low>>shift)&mask)<=((mid>>shift)&mask));
            assert(((mid>>shift)&mask)<=((full>>shift)&mask));
        }
        assert(v1_dim_pixel((uint16_t)pixel,0)==0);
    }
    ui349_contact_t contact={0}; bool raw=true; uint16_t rx=590,ry=20;
    /* A short tap may have only one sampled press between slow frames. */
    ui349_contact(&contact,100000,&raw,&rx,&ry); assert(raw && contact.accepted==1);
    raw=false;rx=ry=0;ui349_contact(&contact,108000,&raw,&rx,&ry);
    assert(raw && rx==590 && ry==20); /* unchanged release bridging */
    raw=false;ui349_contact(&contact,150000,&raw,&rx,&ry);assert(!raw);
    /* Slow polling must not restart qualification or break a held finger. */
    raw=true;rx=400;ry=60;ui349_contact(&contact,200000,&raw,&rx,&ry);assert(raw);
    raw=true;rx=410;ui349_contact(&contact,275000,&raw,&rx,&ry);assert(raw && rx==410);
    raw=true;ui349_contact(&contact,550000,&raw,&rx,&ry);assert(raw && contact.accepted==2);
    raw=false;rx=ry=0;ui349_contact(&contact,599999,&raw,&rx,&ry);assert(raw && rx==410 && ry==60);
    raw=false;ui349_contact(&contact,600000,&raw,&rx,&ry);assert(!raw);
    raw=true;rx=590;ry=20;ui349_contact(&contact,625000,&raw,&rx,&ry);assert(raw && contact.accepted==3);
    raw=false;ui349_contact(&contact,725000,&raw,&rx,&ry);assert(!raw);
    ui349_state_t s={0};
    ui349_configure(&s,(ui349_view_t){200,"OPTIONS",true,0});
    bool covered[TANK_H]={0};
    for (int page=0; page<11; page++) {
        for (int y=0; y<UI349_H; y++) for (int x=0; x<UI349_W; x++) {
            int tx=-1,ty=-1;
            bool hit=ui349_map(&s,x,y,&tx,&ty);
            assert(hit==(x>=96 && x<544));
            if(hit) { assert(tx==x-96 && ty==y+s.scroll); assert(ty<TANK_H); covered[ty]=true; }
        }
        assert(tap(&s,40,130)==UI349_CONSUMED);
    }
    for(int y=0;y<TANK_H;y++) assert(covered[y]);
    assert(s.scroll==UI349_SCROLL_MAX);
    assert(tap(&s,590,30)==UI349_CLOSE);
    assert(tap(&s,590,80)==UI349_SETTINGS);
    assert(tap(&s,590,140)==UI349_SHOP);
    assert(ui349_tap(false,true,600000,12,10));
    assert(!ui349_tap(false,true,900000,0,0));
    assert(!ui349_tap(false,true,100000,30,0));
    assert(ui349_tap(true,false,200000,40,20));
    assert(!ui349_tap(true,false,200000,60,0));
    assert(!ui349_tap(true,false,600000,0,0));
    assert(tap(&s,40,60)==UI349_CONSUMED && s.scroll==UI349_SCROLL_MAX-UI349_SCROLL_STEP);
    ui349_configure(&s,(ui349_view_t){100,"SETUP",false,0});
    assert(s.scroll==0 && tap(&s,590,30)==UI349_CONSUMED);
    assert(tap(&s,590,80)==UI349_CONSUMED);
    assert(tap(&s,590,140)==UI349_CONSUMED);
    ui349_configure(&s,(ui349_view_t){0,"TANK",true,0});
    bool down; int tx,ty;
#ifdef POCKET_TANK_WIDE_V1
    assert(tap(&s,590,20)==UI349_MENU);
    assert(ui349_input(&s,true,570,20,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,true,610,20,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,true,570,20,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,false,0,0,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,true,300,80,&down,&tx,&ty)==UI349_PASS && down);
    assert(ui349_input(&s,true,590,20,&down,&tx,&ty)==UI349_PASS && down);
    assert(ui349_input(&s,false,0,0,&down,&tx,&ty)==UI349_PASS && !down);
    ui349_configure(&s,(ui349_view_t){101,"SETUP",false,0});
    assert(s.positioning);
    assert(tap(&s,40,20)==UI349_CONSUMED && !s.positioning);
    assert(tap(&s,590,30)==UI349_CONSUMED && s.positioning);
    assert(tap(&s,590,20)==UI349_NEXT);
#else
    assert(tap(&s,500,30)==UI349_MENU);
    assert(tap(&s,500,80)==UI349_SETTINGS);
    assert(tap(&s,500,140)==UI349_SHOP);
    assert(ui349_input(&s,true,300,80,&down,&tx,&ty)==UI349_PASS && down);
    assert(ui349_input(&s,true,500,80,&down,&tx,&ty)==UI349_PASS && down);
    assert(tx==TANK_W-1);
    assert(ui349_input(&s,false,0,0,&down,&tx,&ty)==UI349_PASS && !down);
    assert(ui349_input(&s,true,500,30,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,true,300,80,&down,&tx,&ty)==UI349_CONSUMED);
    assert(ui349_input(&s,false,0,0,&down,&tx,&ty)==UI349_CONSUMED);
#endif
    ui349_configure(&s,(ui349_view_t){2584,"INFO",true,184});
    assert(s.scroll==184);
    assert(ui349_map(&s,120,40,&tx,&ty) && ty==224);
    ui349_configure(&s,(ui349_view_t){400,"MENU",true,0});
    assert(s.scroll==0);
    ui349_configure(&s,(ui349_view_t){700,"FISH",true,0});
    assert(tap(&s,590,80)==UI349_SPECIES);
    s.species_picker=true;
    for(int i=0;i<SPECIES_COUNT;i++){s.species_page=i/6;int cell=i%6;assert(tap(&s,150+(cell%2)*224,24+(cell/2)*52)==UI349_SPECIES_PICK+i);}
    assert(tap(&s,590,30)==UI349_SPECIES_BACK);
    assert(!ui349_map(&s,300,80,&tx,&ty));
    ui349_configure(&s,(ui349_view_t){400,"MENU",true,0}); assert(!s.species_picker);
    ui349_configure(&s,(ui349_view_t){0,"TANK",true,0});
    assert(tap(&s,65,24)==UI349_TANKS);
    s.habitat_picker=true;
    for(int h=0;h<HABITAT_COUNT;h++) assert(tap(&s,200,25+h*52)==UI349_HABITAT_PICK+h);
    assert(tap(&s,590,30)==UI349_HABITAT_BACK);
    assert(!ui349_map(&s,300,80,&tx,&ty));
    ui349_configure(&s,(ui349_view_t){700,"FISH",true,0});assert(!s.habitat_picker);
    s.species_picker=true;
    for(int h=0;h<HABITAT_COUNT;h++) {
        s.habitat=h;int row=0;
        for(int sp=0;sp<SPECIES_COUNT;sp++)if(tank_species_allowed(h,sp)){
            s.species_page=row/6;int cell=row++%6;
            assert(tap(&s,150+(cell%2)*224,24+(cell/2)*52)==UI349_SPECIES_PICK+sp);
        }
    }
    s.species_picker=false;
    /* Guard both buffers while composing edge pages and the aquarium. */
    uint16_t *src=calloc(TANK_W*TANK_H,sizeof *src);
    uint16_t *out=calloc(UI349_W*UI349_H+2,sizeof *out);
    tank_t tank={0}; tank.n_fish=N_FISH_MAX;
    for(int i=0;i<N_FISH_MAX;i++) strcpy(tank.fish[i].name,"ABCDEFG");
    assert(src && out); out[0]=0x1234; out[UI349_W*UI349_H+1]=0x5678;
    for(int key=0;key<2;key++) for(int y=0;y<=UI349_SCROLL_MAX;y++) {
        ui349_configure(&s,(ui349_view_t){key,"OPTIONS",true,0}); s.scroll=y;
        ui349_render(&s,&tank,src,out+1);
        assert(out[0]==0x1234 && out[UI349_W*UI349_H+1]==0x5678);
    }
    /* Follow the fish at both edges and in the middle; scrolling its card
     * must not move the camera or corrupt the scene with the card overlay. */
    for(int fy=0;fy<TANK_H;fy+=31) for(int fx=0;fx<TANK_W;fx+=79) {
        for(int i=0;i<TANK_W*TANK_H;i++) src[i]=(uint16_t)(i*13);
        tank.fish[0].x=(float)fx; tank.fish[0].y=(float)fy;
        ui349_configure(&s,(ui349_view_t){700,"FISH",true,0});
        ui349_capture_fish(&s,&tank,src,out+1);
        int nx=244+fx-s.camera_x, ny=fy-s.camera_y;
        assert(nx>=244 && nx<544 && ny>=0 && ny<172);
        assert(ui349_map(&s,nx,ny,&tx,&ty) && tx==fx && ty==fy);
        uint16_t expected=src[fy*TANK_W+fx];
        memset(src,0,TANK_W*TANK_H*sizeof *src); /* later overlay cannot affect camera */
        s.scroll=UI349_SCROLL_MAX;
        ui349_render(&s,&tank,src,out+1);
        assert(out[1+ny*UI349_W+nx]==expected);
        assert(ui349_map(&s,nx,ny,&tx,&ty) && tx==fx && ty==fy);
        assert(out[0]==0x1234 && out[UI349_W*UI349_H+1]==0x5678);
    }
    free(src); free(out);
    puts("PASS: page pixels/touch alignment, full scroll coverage, setup guard, rail release/drag isolation, framebuffer guards");
    return 0;
}
