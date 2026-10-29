#include "time_recovery.h"
#include "sdkconfig.h"
#if CONFIG_POCKET_TANK_WIFI_TIME_RECOVERY
#include "time_recovery_policy.h"
#include "rtc_port.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include <sys/time.h>
#include <string.h>
static const char *TAG="time-recovery";
static portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
static bool active,got_ip,disconnected,accept_sample;
static int64_t sample_epoch,sample_us;
static bool requested,network_ready,wifi_ready,sntp_started;
static int64_t started_us;
static esp_netif_t *station;
static esp_event_handler_instance_t wifi_handler,ip_handler;
static bool wifi_registered,ip_registered;
static void wipe(void *p,size_t n){volatile unsigned char *b=p;while(n--)*b++=0;}
static bool load(time_wifi_credentials_t *c) {
    nvs_handle_t h;memset(c,0,sizeof *c);
    if(nvs_open("time_wifi",NVS_READONLY,&h)!=ESP_OK)return false;
    size_t n=sizeof *c;esp_err_t e=nvs_get_blob(h,"credentials",c,&n);nvs_close(h);
    return e==ESP_OK&&n==sizeof *c&&time_credentials_valid(c);
}
/* Override ESP-IDF's documented weak hook. Never let the TCP/IP task change
 * system time before the tank task has committed an unknown save baseline. */
void sntp_sync_time(struct timeval *tv) {
    portENTER_CRITICAL(&lock);
    if(active&&accept_sample&&!sample_epoch){sample_epoch=tv->tv_sec;sample_us=esp_timer_get_time();}
    portEXIT_CRITICAL(&lock);
}
static void event(void *arg,esp_event_base_t base,int32_t id,void *data) {
    (void)arg;(void)data;
    portENTER_CRITICAL(&lock);
    if(active){if(base==IP_EVENT&&id==IP_EVENT_STA_GOT_IP)got_ip=true;
        if(base==WIFI_EVENT&&id==WIFI_EVENT_STA_DISCONNECTED)disconnected=true;}
    portEXIT_CRITICAL(&lock);
}
void time_recovery_stop(void) {
    portENTER_CRITICAL(&lock);active=false;accept_sample=false;got_ip=false;disconnected=false;sample_epoch=0;portEXIT_CRITICAL(&lock);
    requested=false;
    if(sntp_started){esp_sntp_stop();sntp_started=false;}
    if(wifi_ready){esp_wifi_stop();esp_wifi_deinit();wifi_ready=false;}
    if(ip_registered){esp_event_handler_instance_unregister(IP_EVENT,IP_EVENT_STA_GOT_IP,ip_handler);ip_registered=false;}
    if(wifi_registered){esp_event_handler_instance_unregister(WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,wifi_handler);wifi_registered=false;}
    if(station){esp_netif_destroy_default_wifi(station);station=NULL;}
}
static void start(void) {
    requested=false;time_wifi_credentials_t c;
    if(!load(&c)){wipe(&c,sizeof c);ESP_LOGW(TAG,"clock invalid; Wi-Fi setup required or set clock manually");return;}
    bool ok=true;
    if(!network_ready){ok=esp_netif_init()==ESP_OK;
        if(ok){esp_err_t e=esp_event_loop_create_default();ok=e==ESP_OK||e==ESP_ERR_INVALID_STATE;}
        network_ready=ok;}
    /* The default convenience helper asserts on low memory. Recover safely. */
    esp_netif_config_t netcfg=ESP_NETIF_DEFAULT_WIFI_STA();
    if(ok){station=esp_netif_new(&netcfg);ok=station!=NULL;}
    if(ok)ok=esp_netif_attach_wifi_station(station)==ESP_OK;
    if(ok)ok=esp_wifi_set_default_wifi_sta_handlers()==ESP_OK;
    esp_log_level_set("wifi",ESP_LOG_WARN); /* Avoid network identifiers in normal logs. */
    wifi_init_config_t init=WIFI_INIT_CONFIG_DEFAULT();
    if(ok){ok=esp_wifi_init(&init)==ESP_OK;wifi_ready=ok;}
    if(ok){ok=esp_event_handler_instance_register(WIFI_EVENT,WIFI_EVENT_STA_DISCONNECTED,event,NULL,&wifi_handler)==ESP_OK;wifi_registered=ok;}
    if(ok){ok=esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,event,NULL,&ip_handler)==ESP_OK;ip_registered=ok;}
    wifi_config_t cfg={0};memcpy(cfg.sta.ssid,c.ssid,strlen(c.ssid));memcpy(cfg.sta.password,c.password,strlen(c.password));
    cfg.sta.threshold.authmode=WIFI_AUTH_WPA2_PSK;cfg.sta.pmf_cfg.capable=true;
    if(ok)ok=esp_wifi_set_storage(WIFI_STORAGE_RAM)==ESP_OK&&esp_wifi_set_mode(WIFI_MODE_STA)==ESP_OK&&esp_wifi_set_config(WIFI_IF_STA,&cfg)==ESP_OK;
    wipe(&c,sizeof c);wipe(&cfg,sizeof cfg);
    started_us=esp_timer_get_time();
    portENTER_CRITICAL(&lock);active=ok;portEXIT_CRITICAL(&lock);
    if(ok)ok=esp_wifi_start()==ESP_OK&&esp_wifi_connect()==ESP_OK;
    if(!ok){time_recovery_stop();ESP_LOGW(TAG,"connection could not start; clock unchanged");}
    else ESP_LOGI(TAG,"one time-recovery attempt started (45-second limit)");
}
void time_recovery_init(void){requested=!rtc_port_time_trusted();}
void time_recovery_retry(void){time_recovery_stop();requested=!rtc_port_time_trusted();}
void time_recovery_poll(void) {
    if(rtc_port_time_trusted()){if(wifi_ready||requested)time_recovery_stop();return;}
    if(requested)start();
    portENTER_CRITICAL(&lock);bool running=active,ip=got_ip,lost=disconnected;int64_t epoch=sample_epoch,stamp=sample_us;portEXIT_CRITICAL(&lock);
    if(!running)return;
    int64_t now=esp_timer_get_time();
    if(lost||now-started_us>=45000000){time_recovery_stop();ESP_LOGW(TAG,"time recovery unavailable; clock unchanged, Wi-Fi off");return;}
    if(ip&&!sntp_started){
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);esp_sntp_setservername(0,"time.cloudflare.com");
        portENTER_CRITICAL(&lock);accept_sample=true;portEXIT_CRITICAL(&lock);
        esp_sntp_init();sntp_started=true;
    }
    if(epoch){
        bool valid=time_sample_valid(epoch,stamp,now);time_recovery_stop();
        /* RTC and aquarium writes happen ONLY here, on the tank task. The
         * setter rebases the selected slot before/after setting time; other
         * slots retain their data and rebase when selected. No catch-up. */
        bool ok=valid&&rtc_port_set_utc(epoch+(now-stamp)/1000000);
        ESP_LOGI(TAG,"time recovery %s; Wi-Fi off; no offline catch-up",ok?"complete":"failed");
    }
}
bool time_recovery_configure(const char *ssid,const char *password) {
    time_wifi_credentials_t c;if(!time_credentials_decode(ssid,password,&c))return false;
    nvs_handle_t h;esp_err_t e=nvs_open("time_wifi",NVS_READWRITE,&h);
    if(e==ESP_OK){e=nvs_set_blob(h,"credentials",&c,sizeof c);if(e==ESP_OK)e=nvs_commit(h);nvs_close(h);}
    wipe(&c,sizeof c);if(e!=ESP_OK)return false;
    time_recovery_retry();return true;
}
void time_recovery_status(void) {
    time_wifi_credentials_t c;bool configured=load(&c);wipe(&c,sizeof c);
    ESP_LOGI(TAG,"configured=%d clock_valid=%d attempt_active=%d",configured,rtc_port_time_trusted(),wifi_ready);
}
#else
void time_recovery_init(void){}
void time_recovery_poll(void){}
void time_recovery_stop(void){}
void time_recovery_retry(void){}
void time_recovery_status(void){}
bool time_recovery_configure(const char *s,const char *p){(void)s;(void)p;return false;}
#endif
