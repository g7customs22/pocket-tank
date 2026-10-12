/* V1 only. Complete frame streams, in order, as in Waveshare's IDF example. */
#include "display_port.h"
#include <string.h>
#include "brightness.h"
#include "board_waveshare_349_v1.h"
#include "viewport_349_v1.h"
#include "pixels_349_v1.h"
#include "ui_349_v1.h"
#include "touch_port.h"
#include "battery_port.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_lcd_axs15231b.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define ROWS 64
static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_stripe;
static uint16_t *s_native;
static tank_t *s_tank;
void display349_view(tank_t *tank, ui349_view_t view, const uint16_t *source) {
    s_tank = tank;
    ui349_configure(&g_ui349, view);
    if (s_native) ui349_capture_fish(&g_ui349, tank, source, s_native);
}
static SemaphoreHandle_t s_done;
static bool s_inverted, s_awake;
static uint8_t s_brightness = 255;
static const axs15231b_lcd_init_cmd_t s_init[] = {
    {0x11, NULL, 0, 100}, {0x29, NULL, 0, 100}
};
static bool transferred(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *event, void *ctx) {
    (void)io; (void)event; (void)ctx;
    BaseType_t wake = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &wake);
    return wake == pdTRUE;
}
static void reset_panel(void) {
    ESP_ERROR_CHECK(gpio_set_level(V1_LCD_RST, 1)); vTaskDelay(pdMS_TO_TICKS(30));
    ESP_ERROR_CHECK(gpio_set_level(V1_LCD_RST, 0)); vTaskDelay(pdMS_TO_TICKS(250));
    ESP_ERROR_CHECK(gpio_set_level(V1_LCD_RST, 1)); vTaskDelay(pdMS_TO_TICKS(30));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
}
void display_port_set_brightness(uint8_t level) {
    s_brightness = level;
    if (!s_awake) return;
    /* V1 PWM feeds the boost converter's feedback: vendor LCD_PWM_MODE_255
     * is (0xff - 255), so full brightness requires zero duty. */
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 255u - v1_backlight_drive(level)));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
    ESP_ERROR_CHECK(board_349_v1_output(V1_EX_BL, level != 0));
}
uint8_t display_port_brightness(void) { return s_brightness; }
void display_port_set_inverted(bool inverted) { s_inverted = inverted; }
bool display_port_init(void) {
    s_stripe = heap_caps_malloc(V1_PANEL_W * ROWS * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_done = xSemaphoreCreateBinary();
    s_native = heap_caps_malloc(UI349_W * UI349_H * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!s_stripe || !s_done || !s_native) {
        ESP_LOGE("axs15231b", "allocation failed: DMA=%d semaphore=%d native=%d; internal largest=%u PSRAM largest=%u",
                 s_stripe != NULL, s_done != NULL, s_native != NULL,
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        return false;
    }
    gpio_config_t rst = {.pin_bit_mask = 1ULL << V1_LCD_RST, .mode = GPIO_MODE_OUTPUT};
    ESP_ERROR_CHECK(gpio_config(&rst));
    ledc_timer_config_t timer = {.speed_mode = LEDC_LOW_SPEED_MODE, .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_3, .freq_hz = 50000, .clk_cfg = LEDC_USE_RC_FAST_CLK};
    ledc_channel_config_t channel = {.gpio_num = V1_LCD_BL, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1, .timer_sel = LEDC_TIMER_3, .duty = 255};
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
    spi_bus_config_t bus = AXS15231B_PANEL_BUS_QSPI_CONFIG(V1_LCD_CLK, V1_LCD_D0, V1_LCD_D1,
        V1_LCD_D2, V1_LCD_D3, V1_PANEL_W * ROWS * 2);
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_CH_AUTO));
    esp_lcd_panel_io_spi_config_t io_cfg = {.cs_gpio_num = V1_LCD_CS, .dc_gpio_num = -1,
        .spi_mode = 3, .pclk_hz = 40000000, .trans_queue_depth = 1, .lcd_cmd_bits = 32,
        .lcd_param_bits = 8, .on_color_trans_done = transferred, .flags.quad_mode = true};
    esp_lcd_panel_io_handle_t io;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_cfg, &io));
    static const axs15231b_vendor_config_t vendor = {.init_cmds = s_init,
        .init_cmds_size = sizeof s_init / sizeof s_init[0], .flags.use_qspi_interface = 1};
    esp_lcd_panel_dev_config_t panel = {.reset_gpio_num = -1, .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16, .vendor_config = (void *)&vendor};
    ESP_ERROR_CHECK(esp_lcd_new_panel_axs15231b(io, &panel, &s_panel));
    reset_panel();
    s_awake = true;
    display_port_set_brightness(s_brightness);
    ESP_LOGI("axs15231b", "V1 landscape 640x172; native text shell, 1:1 scrollable menus");
    return true;
}
void display_port_flush(const uint16_t *fb) {
    if (!s_awake) return;
    static int64_t report_at,compose_us,convert_us,submit_us,wait_us,poll_us;
    static unsigned frames;
    int64_t section=esp_timer_get_time();if(!report_at)report_at=section;
    if (s_tank) {
        static int64_t meter_at;
        int64_t meter_now=esp_timer_get_time();
        if(!meter_at||meter_now-meter_at>=1000000){
            if(g_ui349.feed_notice_s)g_ui349.feed_notice_s--;
            ui349_battery_update(&g_ui349.battery,battery_port_vbat_mv());meter_at=meter_now;
        }
        ui349_render(&g_ui349, s_tank, fb, s_native);
    }
    compose_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
    if(s_tank)touch_port_poll(s_tank); /* do not join composition to the first DMA stripe */
    poll_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
    const unsigned gain=v1_image_gain((unsigned)brightness_level());
    /* Small channel tables avoid multiplies per pixel; never modify the source
     * or the saved fish camera, so dimming cannot accumulate between frames. */
    uint16_t rb[32], green[64];
    for(unsigned v=0;v<32;v++) rb[v]=v1_dim_pixel((uint16_t)v,gain);
    for(unsigned v=0;v<64;v++) green[v]=v1_dim_pixel((uint16_t)(v<<5),gain);
    for (int py = 0; py < V1_PANEL_H; py += ROWS) {
        if(s_tank)v1_convert_stripe(s_native,s_stripe,py,ROWS,s_inverted,gain,rb,green);
        else memset(s_stripe,0,V1_PANEL_W*ROWS*sizeof *s_stripe);
        convert_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(s_panel, 0, py, V1_PANEL_W, py + ROWS, s_stripe));
        submit_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
        /* The same buffer is never reused while DMA owns it. A timeout is fatal. */
        if (xSemaphoreTake(s_done, pdMS_TO_TICKS(1000)) != pdTRUE) ESP_ERROR_CHECK(ESP_ERR_TIMEOUT);
        wait_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
        /* The native frame is already composed. Poll on this same tank task
         * between stripes so a short tap cannot hide inside the whole flush. */
        if (s_tank) touch_port_poll(s_tank);
        poll_us+=esp_timer_get_time()-section;section=esp_timer_get_time();
    }
    frames++;
    if(section-report_at>=10000000){
        ESP_LOGI("display349","flush breakdown ms/frame: compose %.2f convert %.2f submit %.2f DMA-wait %.2f touch %.2f (%u frames)",
          compose_us/1000.f/frames,convert_us/1000.f/frames,submit_us/1000.f/frames,wait_us/1000.f/frames,poll_us/1000.f/frames,frames);
        compose_us=convert_us=submit_us=wait_us=poll_us=0;frames=0;report_at=section;
    }
}
void display_port_sleep(void) {
    if (!s_awake) return;
    ESP_ERROR_CHECK(board_349_v1_output(V1_EX_BL, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, false));
    ESP_ERROR_CHECK(gpio_set_level(V1_LCD_RST, 0));
    s_awake = false;
}
void display_port_wake(void) {
    if (s_awake || !s_panel) return;
    reset_panel();
    s_awake = true;
    display_port_set_brightness(s_brightness);
}
