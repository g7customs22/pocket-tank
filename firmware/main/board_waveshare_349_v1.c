#include "board_waveshare_349_v1.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t s_bus, s_touch_bus;
static i2c_master_dev_handle_t s_expander, s_touch;
static SemaphoreHandle_t s_lock;
static uint8_t s_outputs;
/* Console and polling run on the tank task. Disabled unless explicitly armed. */
static int64_t s_trace_until, s_trace_last;
static unsigned s_trace_lines;
static bool s_trace_down;

void board_349_v1_touch_trace(unsigned seconds) {
    if (seconds > 120) seconds = 120;
    s_trace_until = seconds ? esp_timer_get_time() + (int64_t)seconds * 1000000 : 0;
    s_trace_last = 0;
    s_trace_lines = 0;
    s_trace_down = false;
    ESP_LOGI("touchraw", "capture %u seconds (maximum 256 records)", seconds);
}

static void trace_touch(esp_err_t err, const uint8_t *data) {
    if (!s_trace_until) return;
    int64_t now = esp_timer_get_time();
    if (now >= s_trace_until || s_trace_lines >= 256) {
        s_trace_until = 0;
        ESP_LOGI("touchraw", "capture ended: %u records", s_trace_lines);
        return;
    }
    if (err != ESP_OK) {
        if (now - s_trace_last >= 100000) {
            ESP_LOGI("touchraw", "us=%lld read error=%d", (long long)now, (int)err);
            s_trace_last = now;
            ++s_trace_lines;
        }
        return; /* Failed reads have no valid packet and do not imply release. */
    }
    bool down = data[1] > 0 && data[1] < 5;
    if (down != s_trace_down || (down && now - s_trace_last >= 100000)) {
        ESP_LOGI("touchraw", "us=%lld down=%d packet=%02x %02x %02x %02x %02x %02x x=%u y=%u",
                 (long long)now, down, data[0], data[1], data[2], data[3], data[4], data[5],
                 ((data[2] & 15) << 8) | data[3], ((data[4] & 15) << 8) | data[5]);
        s_trace_last = now;
        ++s_trace_lines;
    }
    s_trace_down = down;
}

i2c_master_bus_handle_t board_i2c_bus(void) { return s_bus; }
static esp_err_t reg_write(uint8_t reg, uint8_t value) {
    uint8_t data[] = {reg, value};
    return i2c_master_transmit(s_expander, data, sizeof data, 100);
}
esp_err_t board_349_v1_output(uint8_t mask, bool high) {
    if (!s_lock || (mask & ~(V1_EX_BL | V1_EX_POWER | V1_EX_AMP))) return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    uint8_t next = high ? s_outputs | mask : s_outputs & ~mask;
    esp_err_t err = reg_write(1, next);
    if (err == ESP_OK) s_outputs = next;
    xSemaphoreGive(s_lock);
    return err;
}
esp_err_t board_349_v1_init(void) {
    i2c_master_bus_config_t bus = {.i2c_port = I2C_NUM_0, .sda_io_num = 47, .scl_io_num = 48,
        .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = true};
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus, &s_bus), "board349", "peripheral bus");
    i2c_device_config_t dev = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = 0x20, .scl_speed_hz = 400000};
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev, &s_expander), "board349", "expander");
    uint8_t reg = 1, config;
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(s_expander, &reg, 1, &s_outputs, 1, 100), "board349", "outputs");
    reg = 3;
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(s_expander, &reg, 1, &config, 1, 100), "board349", "directions");
    /* Set latch values before output directions; do not alter unrelated pins. */
    s_outputs = (s_outputs | V1_EX_POWER) & ~(V1_EX_BL | V1_EX_AMP);
    ESP_RETURN_ON_ERROR(reg_write(1, s_outputs), "board349", "latch");
    ESP_RETURN_ON_ERROR(reg_write(3, config & ~(V1_EX_BL | V1_EX_POWER | V1_EX_AMP)), "board349", "directions");
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;
    gpio_config_t key = {.pin_bit_mask = 1ULL << V1_PWR_KEY, .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    ESP_RETURN_ON_ERROR(gpio_config(&key), "board349", "PWR key");
    bus.i2c_port = I2C_NUM_1; bus.sda_io_num = 17; bus.scl_io_num = 18;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus, &s_touch_bus), "board349", "touch bus");
    dev.device_address = 0x3b; dev.scl_speed_hz = 300000;
    return i2c_master_bus_add_device(s_touch_bus, &dev, &s_touch);
}
esp_err_t board_349_v1_touch(bool *down, uint16_t *x, uint16_t *y) {
    static const uint8_t command[11] = {0xb5, 0xab, 0xa5, 0x5a, 0, 0, 0, 0x0e, 0, 0, 0};
    uint8_t data[32];
    esp_err_t err = i2c_master_transmit_receive(s_touch, command, sizeof command, data, sizeof data, 20);
    trace_touch(err, data);
    if (err != ESP_OK) return err; /* Do not turn a bus error into a release/tap. */
    static bool first = true;
    if (first) {
        ESP_LOGI("touch349", "first response: %02x %02x %02x %02x %02x %02x",
                 data[0], data[1], data[2], data[3], data[4], data[5]);
        first = false;
    }
    /* Waveshare's V1 callback treats only counts 1..4 as a press. */
    *down = data[1] > 0 && data[1] < 5;
    *x = ((data[2] & 15) << 8) | data[3];
    *y = ((data[4] & 15) << 8) | data[5];
    if (*down && (*x >= 640 || *y >= 172)) return ESP_ERR_INVALID_RESPONSE;
    return ESP_OK;
}
void board_349_v1_wait_key_release(void) {
    while (!gpio_get_level(V1_PWR_KEY)) vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelay(pdMS_TO_TICKS(30));
}
