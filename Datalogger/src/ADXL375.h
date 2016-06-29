/************************************************************************/
/* @file adxl375.h
/* @brief contains prototype declarations and macros for the ADXL375 accelerometer
/************************************************************************/
#ifndef ADXL375_H_
#define ADXL375_H_

#include <asf.h>

void configure_ADXL375(void);
void ADXL375_begin_sampling(void);
void ADXL375_end_sampling(void);
void ADXL375_read_samples(uint8_t* data, uint8_t length);
void ADXL375_ISR_Handler(void);
void ADXL375_sample(void);
void ADXL375_calibrate(void);
void ADXL375_set_activity_thresh(int8_t threshold, bool x, bool y, bool z);
void ADXL375_set_inactivity_thresh(int8_t threshold, uint8_t duration, bool x, bool y, bool z);
void ADXL375_disable_interrupt(uint8_t interrupt_src);
void ADXL375_enable_interrupt(uint8_t interrupt_src);

uint8_t ADXL375_inactive_interrupts;
uint8_t ADXL375_buffer_full_count;

/* Defines for the ADXL375 temperature sensor */

#define ADXL375_ADDR							0x53

// Offset registers to ensure that the device is zeroed before operation
#define ADXL375_OFSX_ADDR						0x1E
#define ADXL375_OFSY_ADDR						0x1F
#define ADXL375_OFSZ_ADDR						0x20


// Shock duration register -- Amount of time the shock thresh must be me to recognize as a shock event
#define ADXL375_DUR_ADDR						0x21
#define ADXL375_DUR_DISABLE_SHOCK				0x00

// Active inactive thresh regs -- Values for the shock to be above and below for active/inactive to be met
#define ADXL375_THRESH_ACT_ADDR					0x24
#define ADXL375_THRESH_INACT_ADDR				0x25

// Active inactive control regs -- Address of the active inactive control
#define ADXL375_ACT_INACT_CTL_ADDR				0x27
#define ADXL375_ACT_INACT_ACT_AC_COUPLE			0x80		
#define ADXL375_ACT_INACT_ACT_X_EN				0x40
#define ADXL375_ACT_INACT_ACT_Y_EN				0x20
#define ADXL375_ACT_INACT_ACT_Z_EN				0x10
#define ADXL375_ACT_INACT_INACT_AC_COUPLE		0x08
#define ADXL375_ACT_INACT_INACT_X_EN			0x04
#define ADXL375_ACT_INACT_INACT_Y_EN			0x02
#define ADXL375_ACT_INACT_INACT_Z_EN			0x01


// Inactive time reg -- the amount of time that the inactive thresh must be met to recognize as inactive
#define ADXL375_TIME_INACT_ADDR					0x26

// POWER_CTL Register
#define ADXL375_POWER_CTL_ADDR					0x2D
#define ADXL375_POWER_CTL_LINK					0x20
#define ADXL375_POWER_CTL_AUTOSLEEP				0x10
#define ADXL375_POWER_CTL_MEASUSRE				0x08
#define ADXL375_POWER_CTL_SLEEP					0x04
#define ADXL375_POWER_CTL_WAKE_SPEED_1HZ		0x03
#define ADXL375_POWER_CTL_WAKE_SPEED_2HZ		0x02
#define ADXL375_POWER_CTL_WAKE_SPEED_4HZ		0x01
#define ADXL375_POWER_CTL_WAKE_SPEED_8HZ		0x00

// Bandwidth rate control reg
#define ADXL375_BW_RATE_ADDR					0x2C
#define ADXL375_BW_RATE_LOW_PWR					0x10
#define ADXL375_BW_RATE_ONE_TENTH				0x00


// INT_MAP Interrupt Mapping Register
#define ADXL375_INT_MAP_ADDR					0x2F
#define ADXL375_INT_MAP_DATA_READY				0x80
#define ADXL375_INT_MAP_SINGLE_SHOCK			0x40
#define ADXL375_INT_MAP_DOUBLE_SHOCK			0x20
#define ADXL375_INT_MAP_ACTIVITY				0x10
#define ADXL375_INT_MAP_INACTIVITY				0x08
#define ADXL375_INT_MAP_WATERMARK				0x02
#define ADXL375_INT_MAP_OVERRUN					0x01

// INT_SOURCE Interrupt Enable Register
#define ADXL375_INT_EN_ADDR						0x2E
#define ADXL375_INT_EN_DATA_READY				0x80
#define ADXL375_INT_EN_SINGLE_SHOCK				0x40
#define ADXL375_INT_EN_DOUBLE_SHOCK				0x20
#define ADXL375_INT_EN_ACTIVITY					0x10
#define ADXL375_INT_EN_INACTIVITY				0x08
#define ADXL375_INT_EN_WATERMARK				0x02
#define ADXL375_INT_EN_OVERRUN					0x01

// INT_SOURCE Interrupt Source Register
#define ADXL375_INT_SRC_ADDR					0x30
#define ADXL375_INT_SRC_DATA_READY				0x80
#define ADXL375_INT_SRC_SINGLE_SHOCK			0x40
#define ADXL375_INT_SRC_DOUBLE_SHOCK			0x20
#define ADXL375_INT_SRC_ACTIVITY				0x10
#define ADXL375_INT_SRC_INACTIVITY				0x08
#define ADXL375_INT_SRC_WATERMARK				0x02
#define ADXL375_INT_SRC_OVERRUN					0x01

// Data Format Register
#define ADXL375_DATA_FORMAT_ADDR				0x31
#define ADXL375_DATA_FORMAT_SELF_TEST			0x80
#define ADXL375_DATA_FORMAT_SPI					0x40
#define ADXL375_DATA_FORMAT_INT_INVERT			0x20
#define ADXL375_DATA_FORMAT_JUSTIFY				0x04

// FIFO
#define ADXL375_FIFO_ADDR						0x38
#define ADXL375_FIFO_BYPASS						0x00
#define ADXL375_FIFO_FIFO						0x40
#define ADXL375_FIFO_STREAM						0x80
#define ADXL375_FIFO_TRIGGER					0xC0

// FIFO status
#define ADXL375_FIFO_STATUS_ADDR				0x39

// Data regs
#define ADXL375_DATAX0							0x32
#define ADXL375_DATAX1							0x33
#define ADXL375_DATAY0							0x34
#define ADXL375_DATAY1							0x35
#define ADXL375_DATAZ0							0x36
#define ADXL375_DATAZ1							0x37

// Interrupt pin
#define ADXL375_INT_PIN							PIN_PA16

#endif /* ADXL375_H_ */