#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../main/time_recovery.c"
static int64_t tick,committed_epoch,system_epoch;
static bool trusted,configured,commit_ok,set_ok;
static int radio_starts,radio_stops,rtc_calls,sntp_starts,nvs_writes,fail_at,calls;
static time_wifi_credentials_t saved;
static int net,weather_result,weather_begins;
bool weather_client_begin(void){weather_begins++;return true;}
int weather_client_poll(void){return weather_result;}
void weather_client_stop(void){}

static int step(void){return ++calls==fail_at?-1:0;}
int64_t esp_timer_get_time(void){return tick;}
bool rtc_port_time_trusted(void){return trusted;}
bool rtc_port_set_utc(int64_t epoch){rtc_calls++;committed_epoch=epoch;if(set_ok){trusted=true;system_epoch=epoch;}return set_ok;}
esp_err_t esp_netif_init(void){return step();}
esp_err_t esp_event_loop_create_default(void){return step();}
esp_netif_t *esp_netif_new(const esp_netif_config_t*c){return step()?NULL:&net;}
esp_err_t esp_netif_attach_wifi_station(esp_netif_t*p){return step();}
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void){return step();}
void esp_netif_destroy_default_wifi(esp_netif_t*p){assert(p==&net);}
esp_err_t esp_wifi_init(const wifi_init_config_t*c){return step();}
esp_err_t esp_wifi_deinit(void){return 0;}
esp_err_t esp_wifi_stop(void){radio_stops++;return 0;}
esp_err_t esp_wifi_start(void){radio_starts++;return step();}
esp_err_t esp_wifi_connect(void){return step();}
esp_err_t esp_wifi_set_storage(int v){assert(v==WIFI_STORAGE_RAM);return step();}
esp_err_t esp_wifi_set_mode(int v){assert(v==WIFI_MODE_STA);return step();}
esp_err_t esp_wifi_set_config(int v,const wifi_config_t*c){assert(v==WIFI_IF_STA&&c->sta.threshold.authmode==WIFI_AUTH_WPA2_PSK);return step();}
esp_err_t esp_event_handler_instance_register(int a,int b,void(*f)(void*,int,int,void*),void*c,int*d){*d=1;return step();}
esp_err_t esp_event_handler_instance_unregister(int a,int b,int c){return 0;}
void esp_sntp_stop(void){}
void esp_sntp_init(void){sntp_starts++;}
void esp_sntp_setoperatingmode(int v){assert(v==ESP_SNTP_OPMODE_POLL);}
void esp_sntp_setservername(unsigned char i,const char*s){assert(i==0&&!strcmp(s,"time.cloudflare.com"));}
esp_err_t nvs_open(const char*s,int mode,int*h){assert(!strcmp(s,"time_wifi"));*h=1;return 0;}
esp_err_t nvs_get_blob(int h,const char*k,void*out,size_t*n){assert(!strcmp(k,"credentials"));if(!configured)return -1;assert(*n==sizeof saved);memcpy(out,&saved,sizeof saved);return 0;}
esp_err_t nvs_set_blob(int h,const char*k,const void*in,size_t n){assert(!strcmp(k,"credentials")&&n==sizeof saved);memcpy(&saved,in,n);nvs_writes++;return 0;}
esp_err_t nvs_commit(int h){if(commit_ok)configured=true;return commit_ok?0:-1;}
void nvs_close(int h){}
static void fresh(void){time_recovery_stop();network_ready=false;trusted=false;configured=true;commit_ok=set_ok=true;tick=1000;system_epoch=0;radio_starts=radio_stops=rtc_calls=sntp_starts=nvs_writes=fail_at=calls=0;strcpy(saved.ssid,"test-network");strcpy(saved.password,"test-password");}
static void connect_ip(void){time_recovery_init();time_recovery_poll();assert(active);event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);time_recovery_poll();assert(sntp_started);}
int main(void){
 time_wifi_credentials_t c;assert(time_credentials_decode("74657374","3132333435363738",&c));assert(!strcmp(c.ssid,"test")&&!strcmp(c.password,"12345678"));
 assert(!time_credentials_decode("00","3132333435363738",&c));assert(!time_credentials_decode("xyz","3132333435363738",&c));assert(!time_credentials_decode("74","31",&c));
 char maxssid[65],maxpass[127];memset(maxssid,'4',64);maxssid[64]=0;memset(maxpass,'4',126);maxpass[126]=0;assert(time_credentials_decode(maxssid,maxpass,&c));
 fresh();trusted=true;time_recovery_init();time_recovery_poll();assert(!radio_starts&&!rtc_calls);
 fresh();configured=false;time_recovery_init();time_recovery_poll();time_recovery_poll();assert(!radio_starts&&!rtc_calls);
 fresh();connect_ip();struct timeval tv={.tv_sec=1800000000,.tv_usec=0};sntp_sync_time(&tv);assert(system_epoch==0&&!rtc_calls&&!trusted);tick+=2000000;time_recovery_poll();assert(rtc_calls==1&&committed_epoch==1800000002&&trusted&&!wifi_ready&&!active&&radio_stops==1);
 sntp_sync_time(&tv);time_recovery_poll();assert(rtc_calls==1);puts("PASS callback cannot change time; only tank-task transaction commits; late callback ignored; radio off");
 fresh();time_recovery_init();time_recovery_poll();tick+=45000000;time_recovery_poll();assert(!active&&!wifi_ready&&!rtc_calls&&radio_stops==1);time_recovery_poll();assert(radio_starts==1);
 fresh();connect_ip();event(NULL,WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,NULL);time_recovery_poll();assert(!active&&!rtc_calls);
 fresh();connect_ip();time_recovery_stop();sntp_sync_time(&tv);time_recovery_poll();assert(!rtc_calls&&!active);
 fresh();connect_ip();trusted=true;time_recovery_poll();assert(!active&&!rtc_calls);
 fresh();connect_ip();sntp_sync_time(&tv);tick+=5000001;time_recovery_poll();assert(!rtc_calls&&!active);
 fresh();connect_ip();tv.tv_sec=1;sntp_sync_time(&tv);time_recovery_poll();assert(!rtc_calls&&!active);
 fresh();connect_ip();tv.tv_sec=1800000000;set_ok=false;sntp_sync_time(&tv);time_recovery_poll();assert(rtc_calls==1&&!trusted&&!active&&!wifi_ready);
 for(int i=1;i<=13;i++){fresh();fail_at=i;time_recovery_init();time_recovery_poll();assert(!active&&!wifi_ready&&!station&&!rtc_calls);}
 puts("PASS timeout, disconnect, sleep cancellation, manual correction, stale/bad time, RTC failure and every startup failure stop radio without retries or clock changes");
 fresh();trusted=true;time_recovery_sync();time_recovery_poll();assert(active);
 event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);time_recovery_poll();sntp_sync_time(&tv);time_recovery_poll();assert(rtc_calls==1&&trusted&&!active&&!explicit_sync);time_recovery_poll();assert(radio_starts==1);
 fresh();trusted=true;time_recovery_sync();time_recovery_poll();tick+=45000000;time_recovery_poll();assert(trusted&&!active&&!rtc_calls&&!explicit_sync);
 puts("PASS explicit valid-clock sync and timeout preserve bounded behavior");
 fresh();trusted=true;weather_result=0;weather_begins=0;assert(time_recovery_weather());time_recovery_poll();assert(active&&!sntp_started);
 event(NULL,IP_EVENT,IP_EVENT_STA_GOT_IP,NULL);time_recovery_poll();assert(weather_begins==1&&!sntp_started&&!rtc_calls);
 weather_result=1;time_recovery_poll();assert(!active&&!weather_mode&&!rtc_calls);
 fresh();assert(!time_recovery_weather());trusted=true;assert(time_recovery_weather());time_recovery_poll();trusted=false;time_recovery_poll();assert(requested&&!weather_mode&&!active);
 puts("PASS weather shares radio, never sets clock, shuts down and yields to invalid-clock recovery");
 fresh();commit_ok=false;assert(!time_recovery_configure("74657374","3132333435363738"));assert(!requested&&!rtc_calls);
 fresh();assert(time_recovery_configure("74657374","3132333435363738"));assert(requested&&nvs_writes==1);time_recovery_poll();assert(active);time_recovery_stop();
 fresh();memset(saved.password,'x',sizeof saved.password);time_recovery_init();time_recovery_poll();assert(!active&&!radio_starts);
 assert(!time_sample_valid(4102444800LL,0,0));assert(!time_sample_valid(1800000000,10,9));
 puts("PASS credential bounds, corrupt storage, failed credential commit and separate NVS namespace; no aquarium write API used by recovery controller");return 0;
}
