#pragma once
#include <stdint.h>
#include <stddef.h>
typedef int esp_err_t;typedef void *i2c_master_dev_handle_t;typedef void *i2c_master_bus_handle_t;
typedef struct {int dev_addr_length,device_address,scl_speed_hz;} i2c_device_config_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define I2C_ADDR_BIT_LEN_7 7
int i2c_master_transmit_receive(void*,const uint8_t*,size_t,uint8_t*,size_t,int);
int i2c_master_transmit(void*,const uint8_t*,size_t,int);
int i2c_master_bus_add_device(void*,const i2c_device_config_t*,void**);
