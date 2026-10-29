#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../main/weather_client.c"
static bool trusted=true,fail_commit,fail_alloc,network_available=true;
static int requests,allocations,status=200,result,starts;
static esp_http_client_config_t config;
static const char *payload;
void esp_crt_bundle_attach(void){}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t*c){assert(c->is_async&&c->crt_bundle_attach&&c->disable_auto_redirect&&c->timeout_ms<=1500);assert(!strncmp(c->url,"https://",8));config=*c;requests++;return (void*)1;}
esp_err_t esp_http_client_perform(void*p){if(result)return result;esp_http_client_event_t e={HTTP_EVENT_ON_DATA,(int)strlen(payload),(void*)payload};return config.event_handler(&e);}
int esp_http_client_get_status_code(void*p){return status;}
void esp_http_client_cleanup(void*p){}
void *heap_caps_malloc(size_t n,int caps){if(fail_alloc)return NULL;allocations++;assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return malloc(n);}
void heap_caps_free(void*p){allocations--;free(p);}
size_t heap_caps_get_free_size(int c){return 50000;}
size_t heap_caps_get_largest_free_block(int c){return 30000;}
esp_err_t nvs_open(const char*n,int mode,int*h){assert(!strcmp(n,"ambient"));*h=1;return 0;}
esp_err_t nvs_get_blob(int h,const char*k,void*p,size_t*n){return -1;}
esp_err_t nvs_set_blob(int h,const char*k,const void*p,size_t n){assert(!strcmp(k,"settings")&&n==sizeof(ambient_config_t));return 0;}
esp_err_t nvs_commit(int h){return fail_commit?-1:0;}
void nvs_close(int h){}
bool rtc_port_time_trusted(void){return trusted;}
bool time_recovery_weather(void){starts++;return network_available;}
static void zip_ok(void){payload="{\"places\":[{\"place name\":\"Indianapolis\",\"latitude\":\"39.78\",\"longitude\":\"-86.04\"}]}";assert(weather_client_begin());assert(weather_client_poll()==0&&phase==2);}
int main(void){ambient_t a;ambient_port_init(&a);assert(!strcmp(a.config.zip,"46219")&&a.config.timeout==3);
 trusted=false;assert(!weather_client_begin());trusted=true;fail_alloc=true;assert(!weather_client_begin());fail_alloc=false;
 zip_ok();char json[256];snprintf(json,sizeof json,"{\"utc_offset_seconds\":-14400,\"current\":{\"temperature_2m\":68.4,\"weather_code\":2,\"time\":%lld}}",(long long)time(NULL));payload=json;
 assert(weather_client_poll()==1&&a.weather_valid&&a.temperature==68&&a.offset==-14400);weather_client_stop();assert(!allocations&&!a.loading);
 zip_ok();payload="{\"current\":{\"temperature_2m\":null}}";assert(weather_client_poll()==-1&&a.temperature==68);weather_client_stop();
 assert(weather_client_begin());result=ESP_ERR_HTTP_EAGAIN;assert(!weather_client_poll());result=0;status=404;payload="{}";assert(weather_client_poll()==-1);weather_client_stop();status=200;
 assert(weather_client_begin());char huge[8193];memset(huge,'x',8192);huge[8192]=0;payload=huge;assert(weather_client_poll()==-1);weather_client_stop();
 assert(weather_client_begin());payload="{\"places\":[{\"place name\":\"bad\",\"latitude\":\"NaN\",\"longitude\":\"-86\"}]}";assert(weather_client_poll()==-1);weather_client_stop();
 zip_ok();strcpy(a.config.zip,"10001");payload=json;assert(weather_client_poll()==-1);weather_client_stop();
 ambient_port_init(&a);ambient_action(&a,AMBIENT_OPEN);a.draft.timeout=1;ambient_action(&a,AMBIENT_SAVE);fail_commit=true;ambient_port_poll(&a,100);assert(a.editor&&a.save_error&&a.config.timeout==3);fail_commit=false;ambient_action(&a,AMBIENT_SAVE);ambient_port_poll(&a,200);assert(!a.editor&&a.config.timeout==1);
 starts=0;next_attempt=0;ambient_port_poll(&a,1000);assert(starts==1);ambient_port_poll(&a,1001);assert(starts==1);ambient_port_poll(&a,1800001000LL);assert(starts==2);
 a.weather_valid=true;a.fetched=time(NULL)-3601;ambient_port_poll(&a,1800001001LL);assert(a.stale);
 assert(allocations==0);puts("PASS production HTTPS polling, certificate requirement, ZIP/JSON validation, bounds, stale/failed weather, cleanup, settings failure and refresh throttling");return 0;}
