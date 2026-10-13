#ifndef DECOR_MOTION_H
#define DECOR_MOTION_H
#include <math.h>
typedef struct {float phase,lid,big_rise,bob,stroke;int fill;} decor_motion_t;
static inline decor_motion_t decor_motion(float clock,unsigned id){
 float p=fmodf(clock+id*2.713f,12.f);if(p<0)p+=12;
 decor_motion_t m={0};m.phase=p;
 m.lid=p<7?0:p<8?p-7:p<9?1:p<10?10-p:0;
 m.big_rise=p>=8&&p<11?(p-8)*45:-1;
 m.fill=p<7?(int)(p*.7f):0;
 m.bob=sinf(clock*1.1f+id*1.9f)*2.2f;
 m.stroke=sinf(clock*1.8f+id*1.9f)*3.f;return m;
}
#endif
