#ifndef CLOCK_POLICY_H
#define CLOCK_POLICY_H
#include <stdbool.h>
#include <stdint.h>
/* Unknown clocks and implausible gaps must never become simulated care.
 * A genuine absence longer than the cap deliberately resumes without catch-up. */
static inline int64_t clock_offline_seconds(int64_t now, int64_t saved, int64_t cap) {
    if (saved <= 0 || now <= saved || cap <= 0) return 0;
    int64_t span = now - saved;
    return span <= cap ? span : 0;
}
static inline bool clock_rtc_advancing(int64_t first, int64_t second) {
    return first > 0 && second > first && second - first <= 2;
}
static inline int clock_bcd(uint8_t value) {
    return (value & 15) <= 9 && (value >> 4) <= 9 ?
        (value >> 4) * 10 + (value & 15) : -1;
}
/* PCF85063 registers seconds..year; reject oscillator-stop and invalid dates. */
static inline bool clock_rtc_decode(const uint8_t b[7], int out[6]) {
    if ((b[0]&0x80) || (b[1]&0x80) || (b[2]&0xc0) ||
        (b[3]&0xc0) || b[4]>6 || (b[5]&0xe0)) return false;
    int s=clock_bcd(b[0]), m=clock_bcd(b[1]), h=clock_bcd(b[2]);
    int d=clock_bcd(b[3]), mo=clock_bcd(b[5]), y=clock_bcd(b[6]);
    if(s<0||s>59||m<0||m>59||h<0||h>23||d<1||mo<1||mo>12||y<24) return false;
    static const int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(d>days[mo-1]+(mo==2 && y%4==0)) return false;
    out[0]=s;out[1]=m;out[2]=h;out[3]=d;out[4]=mo-1;out[5]=y+100;
    return true;
}
#endif
