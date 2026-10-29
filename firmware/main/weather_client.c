#include "weather_client.h"
#include "time_recovery.h"
#include "rtc_port.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs.h"
#include "cJSON.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* All network and UI state is owned by the tank task. HTTPS perform is polled
 * asynchronously; no worker may mutate an aquarium or set the system clock. */
static ambient_t *ui;
static esp_http_client_handle_t client;
static char *body;
static size_t used;
static bool overflow;
static int phase;
static int64_t next_attempt;
static char request_zip[6],place[32];
static double latitude,longitude;
static esp_err_t receive(esp_http_client_event_t *e){
    if(e->event_id==HTTP_EVENT_ON_DATA){
        if(e->data_len<0||used+(size_t)e->data_len>=8192){overflow=true;return ESP_FAIL;}
        memcpy(body+used,e->data,e->data_len);used+=e->data_len;body[used]=0;
    }
    return ESP_OK;
}
static bool request(const char *url){
    used=0;overflow=false;body[0]=0;
    esp_http_client_config_t cfg={.url=url,.event_handler=receive,.timeout_ms=1500,
        .is_async=true,.crt_bundle_attach=esp_crt_bundle_attach,.buffer_size=1024,
        .buffer_size_tx=512,.disable_auto_redirect=true,.user_agent="PocketTank/1.0"};
    client=esp_http_client_init(&cfg);return client!=NULL;
}
void weather_client_stop(void){
    if(client){esp_http_client_cleanup(client);client=NULL;}
    if(body){heap_caps_free(body);body=NULL;}
    if(ui)ui->loading=false;
    phase=0;
}
bool weather_client_begin(void){
    if(!ui||!rtc_port_time_trusted())return false;
    body=heap_caps_malloc(8192,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(!body)return false;
    memcpy(request_zip,ui->config.zip,6);char url[80];
    snprintf(url,sizeof url,"https://api.zippopotam.us/us/%s",request_zip);
    phase=1;ui->loading=true;return request(url);
}
static bool coordinate(const cJSON *o,const char *name,double lo,double hi,double *out){
    const cJSON *v=cJSON_GetObjectItemCaseSensitive(o,name);
    if(!cJSON_IsString(v))return false;
    char *end;double n=strtod(v->valuestring,&end);
    if(end==v->valuestring||*end||!isfinite(n)||n<lo||n>hi)return false;
    *out=n;return true;
}
int weather_client_poll(void){
    if(!client||!body||!ui)return -1;
    esp_err_t e=esp_http_client_perform(client);
    if(e==ESP_ERR_HTTP_EAGAIN)return 0;
    int status=esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);client=NULL;
    if(e!=ESP_OK||status!=200||overflow)return -1;
    cJSON *root=cJSON_ParseWithLength(body,used+1);if(!root)return -1;
    bool ok=false;
    if(phase==1){
        cJSON *p=cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(root,"places"),0);
        cJSON *name=cJSON_GetObjectItemCaseSensitive(p,"place name");
        ok=coordinate(p,"latitude",-90,90,&latitude)&&coordinate(p,"longitude",-180,180,&longitude)&&cJSON_IsString(name);
        if(ok){snprintf(place,sizeof place,"%.31s",name->valuestring);for(char *c=place;*c;c++)if((unsigned char)*c<32||(unsigned char)*c>126)*c=' ';}
        cJSON_Delete(root);if(!ok)return -1;
        char url[256];snprintf(url,sizeof url,"https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,weather_code&temperature_unit=fahrenheit&timezone=auto&timeformat=unixtime",latitude,longitude);
        phase=2;return request(url)?0:-1;
    }
    cJSON *cur=cJSON_GetObjectItemCaseSensitive(root,"current");
    cJSON *temp=cJSON_GetObjectItemCaseSensitive(cur,"temperature_2m"),*code=cJSON_GetObjectItemCaseSensitive(cur,"weather_code");
    cJSON *stamp=cJSON_GetObjectItemCaseSensitive(cur,"time"),*offset=cJSON_GetObjectItemCaseSensitive(root,"utc_offset_seconds");
    int64_t now=time(NULL);
    ok=cJSON_IsNumber(temp)&&isfinite(temp->valuedouble)&&temp->valuedouble>=-150&&temp->valuedouble<=150&&
       cJSON_IsNumber(code)&&code->valuedouble==code->valueint&&code->valueint>=0&&code->valueint<=99&&
       cJSON_IsNumber(offset)&&offset->valuedouble==offset->valueint&&offset->valueint>=-43200&&offset->valueint<=50400&&
       cJSON_IsNumber(stamp)&&fabs((double)now-stamp->valuedouble)<=7200&&strcmp(request_zip,ui->config.zip)==0;
    if(ok){ui->temperature=(int)lround(temp->valuedouble);ui->code=code->valueint;ui->offset=offset->valueint;
        ui->fetched=now;ui->weather_valid=true;ui->stale=false;memcpy(ui->place,place,sizeof place);
        ESP_LOGI("weather","updated; radio will stop; internal free=%u largest=%u",(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),(unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));}
    cJSON_Delete(root);return ok?1:-1;
}
void ambient_port_init(ambient_t *a){
    ambient_defaults(a);ui=a;next_attempt=0;
    setenv("TZ","EST5EDT,M3.2.0/2,M11.1.0/2",1);tzset();
    nvs_handle_t h;if(nvs_open("ambient",NVS_READONLY,&h)==ESP_OK){
        ambient_config_t c;size_t n=sizeof c;
        if(nvs_get_blob(h,"settings",&c,&n)==ESP_OK&&n==sizeof c&&ambient_config_valid(&c))a->config=c;
        nvs_close(h);
    }
}
void ambient_port_poll(ambient_t *a,int64_t now){
    if(a->pending){
        a->pending=false;nvs_handle_t h;esp_err_t e=nvs_open("ambient",NVS_READWRITE,&h);
        if(e==ESP_OK){e=nvs_set_blob(h,"settings",&a->draft,sizeof a->draft);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);}
        if(e==ESP_OK){bool changed=strcmp(a->config.zip,a->draft.zip)!=0;
            a->config=a->draft;a->editor=false;a->save_error=false;if(changed){a->weather_valid=false;a->place[0]=0;next_attempt=0;}
        }else {a->save_error=true;ESP_LOGW("weather","settings not saved; editor remains open");}
    }
    a->clock_valid=rtc_port_time_trusted();
    int64_t utc=time(NULL);a->stale=a->weather_valid&&(utc<a->fetched||utc-a->fetched>3600);
    if(a->clock_valid){
        struct tm tm;time_t local;
        if(!strcmp(a->config.zip,"46219")){
            /* Indianapolis: Eastern time with US DST rules, even while offline. */
            time_t t=(time_t)utc;localtime_r(&t,&tm);
        }else if(a->weather_valid){local=(time_t)(utc+a->offset);gmtime_r(&local,&tm);}
        else {a->clock_valid=false;memset(&tm,0,sizeof tm);}
        a->pm=tm.tm_hour>=12;a->hour=tm.tm_hour%12;if(!a->hour)a->hour=12;a->minute=tm.tm_min;
    }
    if(a->config.timeout&&rtc_port_time_trusted()&&!a->editor&&now>=next_attempt){
        if(time_recovery_weather()){next_attempt=now+1800000000LL;a->loading=true;}
        else next_attempt=now+30000000;
    }
}
