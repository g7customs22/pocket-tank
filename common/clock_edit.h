#ifndef CLOCK_EDIT_H
#define CLOCK_EDIT_H
#include <stdint.h>
#include <stdbool.h>
/* UTC only, independent of host timezone/DST and mktime. */
static inline bool clock_leap(int y){return y%4==0&&(y%100!=0||y%400==0);}
static inline int clock_days(int y,int m){static const int d[]={31,28,31,30,31,30,31,31,30,31,30,31};return m>=1&&m<=12?d[m-1]+(m==2&&clock_leap(y)):0;}
static inline bool clock_edit_epoch(const int v[6],int64_t *epoch){
    if(v[0]<2024||v[0]>2099||v[1]<1||v[1]>12||v[2]<1||v[2]>clock_days(v[0],v[1])||v[3]<0||v[3]>23||v[4]<0||v[4]>59||v[5]<0||v[5]>59)return false;
    int64_t d=0;for(int y=1970;y<v[0];y++)d+=365+clock_leap(y);
    for(int m=1;m<v[1];m++)d+=clock_days(v[0],m);
    *epoch=((d+v[2]-1)*24+v[3])*3600+v[4]*60+v[5];return true;
}
#endif
