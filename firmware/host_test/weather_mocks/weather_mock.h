#ifndef WEATHER_MOCK_H
#define WEATHER_MOCK_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_HTTP_EAGAIN 2
#define HTTP_EVENT_ON_DATA 3
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_INTERNAL 4
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
typedef int nvs_handle_t;
typedef void *esp_http_client_handle_t;
typedef struct {int event_id,data_len;void *data;} esp_http_client_event_t;
typedef struct {const char *url;esp_err_t(*event_handler)(esp_http_client_event_t*);int timeout_ms;bool is_async;void(*crt_bundle_attach)(void);int buffer_size,buffer_size_tx;bool disable_auto_redirect;const char *user_agent;} esp_http_client_config_t;
void esp_crt_bundle_attach(void);
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t*);
esp_err_t esp_http_client_perform(esp_http_client_handle_t);
int esp_http_client_get_status_code(esp_http_client_handle_t);
void esp_http_client_cleanup(esp_http_client_handle_t);
void *heap_caps_malloc(size_t,int);
void heap_caps_free(void*);
size_t heap_caps_get_free_size(int);
size_t heap_caps_get_largest_free_block(int);
esp_err_t nvs_open(const char*,int,nvs_handle_t*);
esp_err_t nvs_get_blob(nvs_handle_t,const char*,void*,size_t*);
esp_err_t nvs_set_blob(nvs_handle_t,const char*,const void*,size_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
bool rtc_port_time_trusted(void);
static struct tm *mock_localtime(const time_t *t,struct tm *out){return localtime_s(out,t)?NULL:out;}
static struct tm *mock_gmtime(const time_t *t,struct tm *out){return gmtime_s(out,t)?NULL:out;}
#define localtime_r mock_localtime
#define gmtime_r mock_gmtime
#define setenv(a,b,c) _putenv_s(a,b)
#define tzset _tzset
#endif
