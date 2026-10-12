/* V1 has a voltage divider and power latch, not an AXP2101 fuel gauge. */
#include "battery_port.h"
#include "board_waveshare_349_v1.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cal;
static bool s_armed, s_long;
static int64_t s_pressed, s_last_read;
static int s_mv;

bool battery_port_init(i2c_master_bus_handle_t bus) {
    (void)bus;
    adc_oneshot_unit_init_cfg_t unit = {.unit_id = ADC_UNIT_1};
    adc_oneshot_chan_cfg_t chan = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    adc_cali_curve_fitting_config_t cal = {.unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_3,
        .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    esp_err_t err = adc_oneshot_new_unit(&unit, &s_adc);
    if (err == ESP_OK) err = adc_oneshot_config_channel(s_adc, ADC_CHANNEL_3, &chan);
    if (err == ESP_OK) err = adc_cali_create_scheme_curve_fitting(&cal, &s_cal);
    if (err != ESP_OK) ESP_LOGW("battery349", "voltage unavailable: %s", esp_err_to_name(err));
    ESP_LOGI("battery349", "V1 power key/latch ready; no percentage or charge-state sensor");
    return true; /* Power-key capability; meter availability comes from read(). */
}
bool battery_port_read(float *frac, bool *charging) { (void)frac; (void)charging; return false; }
int battery_port_state(void) { return BAT_ON_BATTERY; } /* Ignored when read() is false. */
int battery_port_vbat_mv(void) {
    if (!s_cal) return 0;
    int64_t now = esp_timer_get_time();
    if (s_last_read && now - s_last_read < 1000000) return s_mv;
    s_last_read = now;
    /* V1 schematic: VBAT -> R3 200k -> GPIO4/ADC1_CH3 -> R13 100k -> GND.
     * Match the official calibrated x3 conversion; average eight samples at
     * most once per second. Failed reads invalidate, never retain stale data. */
    int total = 0;
    for (int i=0; i<8; i++) {
        int raw, mv;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_3, &raw) != ESP_OK ||
            adc_cali_raw_to_voltage(s_cal, raw, &mv) != ESP_OK) { s_mv=0; return 0; }
        total += mv;
    }
    s_mv = (total * 3 + 4) / 8;
    return s_mv;
}
void battery_port_key_init(void) { s_armed = false; s_pressed = 0; s_long = false; }
int battery_port_key_poll(void) {
    int64_t now = esp_timer_get_time();
    if (gpio_get_level(V1_PWR_KEY)) {
        int event = s_armed && s_pressed && !s_long && now - s_pressed >= 50000 ? 1 : 0;
        s_armed = true; s_pressed = 0; s_long = false;
        return event;
    }
    if (!s_armed) return 0;
    if (!s_pressed) s_pressed = now;
    if (!s_long && now - s_pressed >= 1500000) { s_long = true; return 2; }
    return 0;
}
bool battery_port_poweroff(void) {
    board_349_v1_wait_key_release();
    return board_349_v1_output(V1_EX_POWER, false) == ESP_OK;
}
void battery_port_key_trace(int seconds) {
    int64_t end = esp_timer_get_time() + (int64_t)seconds * 1000000;
    while (esp_timer_get_time() < end) {
        int event = battery_port_key_poll();
        if (event) ESP_LOGI("battery349", "PWR %s", event == 1 ? "short" : "long");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
void battery_port_dump(void) { ESP_LOGI("battery349", "VBAT %d mV; percentage/charging unavailable", battery_port_vbat_mv()); }
bool battery_port_set_rail(const char *name, bool on) { (void)name; (void)on; return false; }
void battery_port_trim_rails(void) {} /* Never issue AXP register writes on V1. */
