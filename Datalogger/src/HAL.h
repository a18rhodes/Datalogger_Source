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

#define TEMPERATURE_DESCRIPTOR 0x0
#define ACCEL_DESCRIPTOR 0x1
#define LNIBBLE(x) 0x0F & x
#define UNIBBLE(x) 0xF0 & x
#define INACTIVE_MODE 1
#define ACTIVE_MODE 0
#define STATIONARY_MODE 1
#define MOTION_MODE 0
#define ACCEL_BUFFER_SIZE 999
#define TEMP_BUFFER_SIZE 72

void configure_i2c(void);
void configure_mag_sw_int(void (*callback)(void));
void configure_sleepmode(void);
void sleep(void);
void configure_rtc(void);
void rtc_match_callback(void);
void offload_data(void);
void configure_databuffers(void);
void get_timestamp(uint8_t * ucTimestampVector);

void extint_callback(void);

// Temperature threshold to determine active/inactive modes
uint8_t ucActivityTemperatureThreshold;
uint8_t ucInactivityTemperatureThreshold;

// The current mode of operation 1 = inactive 0 = active
uint8_t ucActiveInactive_Mode;
uint8_t ucMotion_State;

/*Data buffer Variables*/

// Temperature array
int16_t uiTemperatureArray[TEMP_BUFFER_SIZE];
uint8_t ucTemperatureArrayPtr;
// Allow for up to 1 datapoint data sets as worst case buffer sizes
int8_t cTemperatureDataSetPtr[TEMP_BUFFER_SIZE];
uint8_t ucTemperatureTimestamps[TEMP_BUFFER_SIZE * 4];
uint8_t ucTemperatureDataSets;

// Accelerometer matrix (3 rows per sample x,y,z)
int8_t ucAccelerometerMatrix[ACCEL_BUFFER_SIZE];
uint16_t uiAccelerometerMatrixPtr;
// Allow for up to 1 datapoint data sets
int16_t iAccelerometerDataSetPtr[ACCEL_BUFFER_SIZE];
uint8_t ucAccelTimestamps[ACCEL_BUFFER_SIZE * 4];
uint16_t uiAccelerometerDataSets;
/*End data buffer variables */

// General status return value used all over the place
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