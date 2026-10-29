#ifndef TIME_RECOVERY_POLICY_H
#define TIME_RECOVERY_POLICY_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef struct { char ssid[33], password[64]; } time_wifi_credentials_t;
static inline int time_hex_digit(char c) {
    return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
}
static inline bool time_unhex(const char *hex,char *out,size_t cap,size_t min) {
    size_t len=strlen(hex);if(len%2||len/2<min||len/2>=cap)return false;
    for(size_t i=0;i<len/2;i++) {
        int a=time_hex_digit(hex[i*2]),b=time_hex_digit(hex[i*2+1]);
        if(a<0||b<0||!(a*16+b))return false;
        out[i]=(char)(a*16+b);
    }
    out[len/2]=0;return true;
}
static inline bool time_credentials_decode(const char *ssid,const char *pass,time_wifi_credentials_t *out) {
    memset(out,0,sizeof *out);
    bool ok=time_unhex(ssid,out->ssid,sizeof out->ssid,1)&&time_unhex(pass,out->password,sizeof out->password,8);
    if(!ok)memset(out,0,sizeof *out);
    return ok;
}
static inline bool time_credentials_valid(const time_wifi_credentials_t *c) {
    return memchr(c->ssid,0,sizeof c->ssid)&&c->ssid[0]&&
           memchr(c->password,0,sizeof c->password)&&strlen(c->password)>=8;
}
/* Plausibility, not authentication: ordinary SNTP cannot prove server identity.
 * Never use stale callbacks or dates outside the RTC's supported calendar. */
static inline bool time_sample_valid(int64_t epoch,int64_t sampled_us,int64_t now_us) {
    return epoch>=1704067200LL&&epoch<4102444800LL&&sampled_us>=0&&
           now_us>=sampled_us&&now_us-sampled_us<=5000000;
}
#endif
