/************************************************************************/
/* @file adt7420.h
/* @brief contains register addresses and prototype declarations for the ADT7420
/************************************************************************/

#ifndef ADT7420_H_
#define ADT7420_H_

#define ADT7420_EN_PIN						PIN_PA15

#define TEMP_SENSOR_ADDRESS					0x48
#define TEMP_SENSOR_TEMP_REG_MS_ADDR		0x00
#define TEMP_SENSOR_TEMP_REG_LS_ADDR		0x01
#define TEMP_SENSOR_STATUS_ADDR				0x02 
#define TEMP_SENSOR_CONFIG_ADDR				0x03

#define TEMP_SESNOR_SFTW_RESET_ADDR			0x2F

#define TEMP_SENSOR_CONFIG_RESOLUTION_13	0x00
#define TEMP_SENSOR_CONFIG_RESOLUTION_12	0x80
#define TEMP_SENSOR_CONFIG_OP_MODE_CONT		0x00
#define TEMP_SENSOR_CONFIG_OP_MODE_OS		0x20
#define TEMP_SENSOR_CONFIG_OP_MODE_SPS		0x40
#define TEMP_SENSOR_CONFIG_OP_MODE_SHDN		0x60
#define TEMP_SENSOR_CONFIG_INT_MODE			0x00
#define TEMP_SENSOR_CONFIG_COMP_MODE		0x10
#define TEMP_SENSOR_CONFIG_INT_POL_LOW		0x00
#define TEMP_SENSOR_CONFIG_INT_POL_HIGH		0x08
#define TEMP_SENSOR_CONFIG_COMP_POL_LOW		0x00
#define TEMP_SENSOR_CONFIG_COMP_POL_HIGH	0x04
#define TEMP_SENSOR_CONFIG_FLT_QUE_1		0x00
#define TEMP_SENSOR_CONFIG_FLT_QUE_2		0x01
#define TEMP_SENSOR_CONFIG_FLT_QUE_3		0x02
#define TEMP_SENSOR_CONFIG_FLT_QUE_4		0x03

// Temp Sensor
void configure_ADT7420(void);
void ADT7420_read_temp(void);

#endif