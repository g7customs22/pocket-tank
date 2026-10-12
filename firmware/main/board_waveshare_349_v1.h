#ifndef BOARD_WAVESHARE_349_V1_H
#define BOARD_WAVESHARE_349_V1_H
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#define V1_LCD_CS 9
#define V1_LCD_CLK 10
#define V1_LCD_D0 11
#define V1_LCD_D1 12
#define V1_LCD_D2 13
#define V1_LCD_D3 14
#define V1_LCD_RST 21
#define V1_LCD_BL 8
#define V1_PWR_KEY 16
#define V1_EX_BL (1u << 1)
#define V1_EX_POWER (1u << 6)
#define V1_EX_AMP (1u << 7)
/* Source: Waveshare V1 schematic and examples, def6edd0b6e1925ed09702eed01a2f181afdf8c1. */
esp_err_t board_349_v1_init(void);
i2c_master_bus_handle_t board_i2c_bus(void);
esp_err_t board_349_v1_output(uint8_t mask, bool high);
esp_err_t board_349_v1_touch(bool *down, uint16_t *x, uint16_t *y);
void board_349_v1_wait_key_release(void);
#endif
