#ifndef TIME_RECOVERY_MOCK_H
#define TIME_RECOVERY_MOCK_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
#define CONFIG_POCKET_TANK_WIFI_TIME_RECOVERY 1
#define portMUX_TYPE int
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(x) ((void)(x))
#define portEXIT_CRITICAL(x) ((void)(x))
#define ESP_LOG_WARN 2
#define esp_log_level_set(a,b) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_ERR_INVALID_STATE 2
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define WIFI_STORAGE_RAM 0
#define WIFI_MODE_STA 1
#define WIFI_IF_STA 0
#define WIFI_AUTH_WPA2_PSK 3
#define WIFI_EVENT_STA_DISCONNECTED 1
#define IP_EVENT_STA_GOT_IP 2
#define ESP_SNTP_OPMODE_POLL 0
#define WIFI_EVENT 10
#define IP_EVENT 11
typedef int esp_event_base_t;
typedef int esp_event_handler_instance_t;
typedef int esp_netif_t;
typedef int nvs_handle_t;
typedef int wifi_init_config_t;
#define WIFI_INIT_CONFIG_DEFAULT() 0
typedef struct {struct {unsigned char ssid[32],password[64];struct {int authmode;} threshold;struct {bool capable;} pmf_cfg;} sta;} wifi_config_t;
int64_t esp_timer_get_time(void);
esp_err_t esp_netif_init(void);
esp_err_t esp_event_loop_create_default(void);
typedef int esp_netif_config_t;
#define ESP_NETIF_DEFAULT_WIFI_STA() 0
esp_netif_t *esp_netif_new(const esp_netif_config_t*);
esp_err_t esp_netif_attach_wifi_station(esp_netif_t*);
esp_err_t esp_wifi_set_default_wifi_sta_handlers(void);
void esp_netif_destroy_default_wifi(esp_netif_t*);
esp_err_t esp_wifi_init(const wifi_init_config_t*);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_set_storage(int);
esp_err_t esp_wifi_set_mode(int);
esp_err_t esp_wifi_set_config(int,const wifi_config_t*);
esp_err_t esp_event_handler_instance_register(int,int,void(*)(void*,int,int,void*),void*,esp_event_handler_instance_t*);
esp_err_t esp_event_handler_instance_unregister(int,int,esp_event_handler_instance_t);
void esp_sntp_stop(void);
void esp_sntp_init(void);
void esp_sntp_setoperatingmode(int);
void esp_sntp_setservername(unsigned char,const char*);
esp_err_t nvs_open(const char*,int,nvs_handle_t*);
esp_err_t nvs_get_blob(nvs_handle_t,const char*,void*,size_t*);
esp_err_t nvs_set_blob(nvs_handle_t,const char*,const void*,size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
bool rtc_port_time_trusted(void);
bool rtc_port_set_utc(int64_t);
#endif
