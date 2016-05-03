/************************************************************************/
/* @file hal.h
/* @brief contains prototype declarations, and global variables
/************************************************************************/

#ifndef HAL_H_
#define HAL_H_

#include "ADT7420.h"
#include "ADXL375.h"
#include "SP1ML.h"
#include "S70FL01.h"

#define TEMPERATURE_DESCRIPTOR 0x00
#define NIBBLE(x) 4*x
#define WINTER_MODE 1
#define SUMMER_MODE 0

void configure_i2c(void);
void configure_mag_sw_int(void (*callback)(void));
void configure_sleepmode(void);
void sleep(void);
void configure_rtc(void);
void rtc_match_callback(void);
void offload_accelerometer_data(void);
void offload_temperature_data(void);
void offload_data(void);

void extint_callback(void);

// The current mode of operation 1 = winter 0 = summer
uint8_t ucCurrent_Mode;

// Temperature array -- This is large enough to buffer 1 whole day in summer mode
int16_t uiTemperatureArray[72];
uint8_t ucTemperatureArrayPtr;

// Accelerometer matrix (3 rows per sample)
uint8_t ucAccelerometerMatrix[3072];
int16_t uiAccelerometerMatrixPtr;

// General status return value
volatile enum status_code status;

// I2C module instance
struct i2c_master_module i2c_master_instance;
struct i2c_master_config config_i2c_master;
bool i2c_enabled;

// SPI related modules
struct spi_module spi_master_instance;
struct spi_slave_inst slave;
struct spi_module spi_slave_inst;
bool spi_enabled;

// UART relate modules
struct usart_config config_usart;
struct usart_module usart_instance;
bool usart_enabled;

// Timer Counter for energy monitoring
struct tc_module tc_instance_cap;
struct tc_config config_tc;

// RTC Module
struct rtc_module rtc_instance;
struct rtc_calendar_alarm_time alarm;

// Memory module
uint8_t S70FL01_active_die;
uint32_t S70FL01_address;

#endif