/* Pure pixel operations: no allocation, device access, or gesture changes. */
#ifndef PIXELS_349_V1_H
#define PIXELS_349_V1_H
#include <stdint.h>
#include <stdbool.h>
#include "viewport_349_v1.h"
static inline void v1_convert_stripe(const uint16_t *native,uint16_t *stripe,int py,int rows,bool inverted,unsigned gain,const uint16_t rb[32],const uint16_t green[64]) {
    /* Read adjacent PSRAM pixels in 16x16 tiles; destination is internal DMA RAM. */
    for(int y0=0;y0<V1_PANEL_W;y0+=16)for(int r0=0;r0<rows;r0+=16){
        int ye=y0+16<V1_PANEL_W?y0+16:V1_PANEL_W;
        int re=r0+16<rows?r0+16:rows;
        for(int y=y0;y<ye;y++){
            const uint16_t *line=native+(inverted?V1_PANEL_W-1-y:y)*V1_PANEL_H;
            for(int row=r0;row<re;row++){
                int x=inverted?py+row:V1_PANEL_H-1-py-row;
                uint16_t p=line[x];
                if(gain!=256)p=(uint16_t)((rb[p>>11]<<11)|green[(p>>5)&63]|rb[p&31]);
                stripe[row*V1_PANEL_W+y]=(uint16_t)((p<<8)|(p>>8));
            }
        }
    }
}
static inline void v1_compose_world(const uint16_t *source,uint16_t *native){
    static uint16_t xm[V1_VIEW_W],ym[V1_PANEL_W];static bool ready;
    if(!ready){
        for(int x=0;x<V1_VIEW_W;x++)xm[x]=(uint16_t)(x*(TANK_W-1)/(V1_VIEW_W-1));
        for(int y=0;y<V1_PANEL_W;y++)ym[y]=(uint16_t)(y*(TANK_H-1)/(V1_PANEL_W-1));
        ready=true;
    }
    for(int y=0;y<V1_PANEL_W;y++){
        const uint16_t *line=source+ym[y]*TANK_W;uint16_t *out=native+y*V1_PANEL_H+V1_VIEW_X;
        for(int x=0;x<V1_VIEW_W;x++)out[x]=line[xm[x]];
    }
}
#endif
