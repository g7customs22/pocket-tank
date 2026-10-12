/* Only a valid, advancing hardware clock may supply offline-care time.
 * Never seed the RTC from build time: battery removal is an unknown interval. */
#include "rtc_port.h"
#include "clock_policy.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
static const char *TAG="rtc";
static i2c_master_dev_handle_t s_dev;
static bool s_trusted;
bool rtc_port_time_trusted(void) { return s_trusted; }
static bool rtc_read(time_t *epoch) {
    uint8_t reg=0x04,b[7]; int v[6];
    if(i2c_master_transmit_receive(s_dev,&reg,1,b,7,100)!=ESP_OK ||
       !clock_rtc_decode(b,v)) return false;
    struct tm t={0};
    t.tm_sec=v[0];t.tm_min=v[1];t.tm_hour=v[2];
    t.tm_mday=v[3];t.tm_mon=v[4];t.tm_year=v[5];
    *epoch=mktime(&t);return *epoch>0;
}
bool rtc_port_init(i2c_master_bus_handle_t bus) {
    s_trusted=false;
    i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
        .device_address=0x51,.scl_speed_hz=400000};
    if(!bus || i2c_master_bus_add_device(bus,&cfg,&s_dev)!=ESP_OK) {
        ESP_LOGW(TAG,"no RTC; offline care disabled");return false;
    }
    time_t first,second;
    if(!rtc_read(&first)) {
        ESP_LOGW(TAG,"RTC unset/invalid; offline care disabled; clock unchanged");return false;
    }
    /* Boot-only check also rejects a stopped clock with a plausible calendar. */
    vTaskDelay(pdMS_TO_TICKS(1100));
    if(!rtc_read(&second) || !clock_rtc_advancing(first,second)) {
        ESP_LOGW(TAG,"RTC not advancing reliably; offline care disabled");return false;
    }
    struct timeval tv={.tv_sec=second};
    if(settimeofday(&tv,NULL)!=0) return false;
    s_trusted=true;
    ESP_LOGI(TAG,"system clock set from advancing RTC");return true;
}
