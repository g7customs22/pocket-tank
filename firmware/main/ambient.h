#ifndef AMBIENT_H
#define AMBIENT_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef struct { uint8_t timeout; char zip[6]; } ambient_config_t;
typedef struct {
    ambient_config_t config,draft;
    bool active,contact,editor,pending,save_error,clock_valid,pm,weather_valid,stale,loading;
    int digit,hour,minute,temperature,code,offset;
    int64_t fetched;
    char place[32];
} ambient_t;
enum { AMBIENT_OPEN=800,AMBIENT_CANCEL,AMBIENT_SAVE,AMBIENT_TIMEOUT,AMBIENT_MINUS,AMBIENT_PLUS,AMBIENT_DIGIT=810 };
static inline unsigned ambient_seconds(unsigned choice){
    static const unsigned seconds[]={0,60,120,300,600,1800};
    return choice<6?seconds[choice]:300;
}
static inline bool ambient_config_valid(const ambient_config_t *c){
    if(c->timeout>=6||c->zip[5])return false;
    for(int i=0;i<5;i++)if(c->zip[i]<'0'||c->zip[i]>'9')return false;
    return true;
}
static inline void ambient_defaults(ambient_t *a){memset(a,0,sizeof *a);a->config.timeout=3;memcpy(a->config.zip,"46219",6);}
static inline void ambient_idle(ambient_t *a,int64_t now,int64_t last,bool eligible){
    unsigned seconds=ambient_seconds(a->config.timeout);
    a->active=eligible&&!a->editor&&!a->contact&&seconds&&now>=last&&now-last>=(int64_t)seconds*1000000;
}
/* Consume the wake press, drag and release before any aquarium hit testing. */
static inline bool ambient_contact(ambient_t *a,bool down){
    if(a->active&&down){a->active=false;a->contact=true;}
    if(!a->contact)return false;
    if(!down)a->contact=false;
    return true;
}
static inline void ambient_action(ambient_t *a,int action){
    if(action==AMBIENT_OPEN){a->draft=a->config;a->editor=true;a->digit=0;a->save_error=false;}
    else if(action==AMBIENT_CANCEL)a->editor=false;
    else if(action==AMBIENT_SAVE){if(ambient_config_valid(&a->draft))a->pending=true;}
    else if(action==AMBIENT_TIMEOUT)a->draft.timeout=(a->draft.timeout+1)%6;
    else if(action>=AMBIENT_DIGIT&&action<AMBIENT_DIGIT+5)a->digit=action-AMBIENT_DIGIT;
    else if(action==AMBIENT_PLUS||action==AMBIENT_MINUS){int n=a->draft.zip[a->digit]-'0';a->draft.zip[a->digit]=(char)('0'+(n+(action==AMBIENT_PLUS?1:9))%10);}
}
static inline const char *ambient_condition(int code){
    if(code==0)return "CLEAR";
    if(code<=3&&code>=1)return "CLOUDY";
    if(code==45||code==48)return "FOG";
    if(code>=51&&code<=57)return "DRIZZLE";
    if((code>=61&&code<=67)||(code>=80&&code<=82))return "RAIN";
    if((code>=71&&code<=77)||code==85||code==86)return "SNOW";
    if(code==95||code==96||code==99)return "THUNDERSTORM";
    return "WEATHER";
}
#endif
