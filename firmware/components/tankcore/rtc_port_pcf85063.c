/* Only a valid, advancing hardware clock may supply offline-care time.
 * Never seed the RTC from build time: battery removal is an unknown interval. */
#include "rtc_port.h"
#include "clock_policy.h"
#include "clock_edit.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "slot_system.h"
static const char *TAG="rtc";
static i2c_master_dev_handle_t s_dev;
static bool s_trusted;
bool rtc_port_time_trusted(void) { return s_trusted; }
void rtc_port_diagnose(void) {
    uint8_t reg=0,b[11]={0};
    esp_err_t e=s_dev?i2c_master_transmit_receive(s_dev,&reg,1,b,sizeof b,100):ESP_FAIL;
    int v[6];
    ESP_LOGI(TAG,"status io=%d trusted=%d ctrl=%02x %02x raw=%02x %02x %02x %02x %02x %02x %02x OS=%d STOP=%d valid=%d system=%lld",
        e,s_trusted,b[0],b[1],b[4],b[5],b[6],b[7],b[8],b[9],b[10],!!(b[4]&128),!!(b[0]&32),e==ESP_OK&&clock_rtc_decode(b+4,v),(long long)time(NULL));
}
static bool rtc_read(time_t *epoch) {
    uint8_t reg=0,b[11]; int v[6];int64_t value;
    if(i2c_master_transmit_receive(s_dev,&reg,1,b,sizeof b,100)!=ESP_OK ||
       (b[0]&0x28) || !clock_rtc_decode(b+4,v)) return false;
    int date[]={v[5]+1900,v[4]+1,v[3],v[2],v[1],v[0]};
    if(!clock_edit_epoch(date,&value))return false;
    *epoch=(time_t)value;return true;
}
bool rtc_port_init(i2c_master_bus_handle_t bus) {
    s_trusted=false;
    i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
        .device_address=0x51,.scl_speed_hz=400000};
    if(!bus || i2c_master_bus_add_device(bus,&cfg,&s_dev)!=ESP_OK) {
        ESP_LOGW(TAG,"no RTC; offline care disabled");return false;
    }
    rtc_port_diagnose();
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

/* Tank-task only. Unknown timestamp is committed BEFORE any clock write,
 * so even power loss mid-operation cannot turn a clock correction into care.
 * Inactive slots are frozen and their timestamps refresh on activation. */
bool rtc_port_set_utc(int64_t epoch) {
#ifndef POCKET_TANK_WIDE_V1
    (void)epoch;return false; /* Baseline transaction currently supports V1 slots. */
#else
    time_t target=(time_t)epoch; struct tm t;
    if(!s_dev || !gmtime_r(&target,&t) || t.tm_year<124 || t.tm_year>199) return false;
    uint8_t reg=0,old[11],w[8]={4};
    if(i2c_master_transmit_receive(s_dev,&reg,1,old,sizeof old,100)!=ESP_OK || (old[0]&0x28))return false;
    /* Reject stopped or 12-hour mode; never alter unrelated control bits. */
    int v[]={t.tm_sec,t.tm_min,t.tm_hour,t.tm_mday,t.tm_wday,t.tm_mon+1,t.tm_year-100};
    for(int i=0;i<7;i++)w[i+1]=(uint8_t)((v[i]/10)*16+v[i]%10);
    struct timeval prior;gettimeofday(&prior,NULL);
    bool was_trusted=s_trusted;s_trusted=false;
    if(!slot_system_save()){s_trusted=was_trusted;return false;}
    time_t a=0,b=0;
    bool ok=i2c_master_transmit(s_dev,w,sizeof w,100)==ESP_OK && rtc_read(&a) && a>=target && a-target<=1;
    if(ok){vTaskDelay(pdMS_TO_TICKS(1100));ok=rtc_read(&b)&&clock_rtc_advancing(a,b);}
    if(ok){struct timeval tv={.tv_sec=b};ok=settimeofday(&tv,NULL)==0;}
    if(!ok){
        s_trusted=false;w[0]=4;memcpy(w+1,old+4,7);
        bool restored=i2c_master_transmit(s_dev,w,sizeof w,100)==ESP_OK;
        uint8_t check[7],start=4;
        restored=restored&&i2c_master_transmit_receive(s_dev,&start,1,check,sizeof check,100)==ESP_OK&&memcmp(check,old+4,7)==0;
        settimeofday(&prior,NULL);
        ESP_LOGE(TAG,"clock set failed; RTC rollback write=%d; offline care disabled; zero baseline retained",restored);
        return false;
    }
    s_trusted=true;
    if(!slot_system_save()){
        /* A failed commit may be ambiguous: keep the new RTC, since either
         * the zero baseline or new timestamp is safe with this clock. */
        s_trusted=false;
        ESP_LOGE(TAG,"UTC set but baseline commit failed; offline care disabled; do not retry blindly");
        return false;
    }
    ESP_LOGI(TAG,"UTC clock set=%lld; baseline saved; no offline catch-up",(long long)b);
    return true;
#endif
}
