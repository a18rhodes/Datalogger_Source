/************************************************************************/
/*  @file ADT7420.c
/*	@Brief ADT7420 Temperature sensor driver 
/************************************************************************/
	
#include <asf.h>
#include "HAL.h"

/************************************************************************/
/* @brief configure_ADT7420 Configures the ADT7420 Temperature sensor module, and the
/* Micrel switch that controls its power domain.
/* @param none
/* @returns none
/************************************************************************/

void configure_ADT7420(void)
{
	// Create an i2c pacekt struct and populate it with the address of the configuration register and the shutdown operating mode value
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer[2] = {TEMP_SENSOR_CONFIG_ADDR,TEMP_SENSOR_CONFIG_OP_MODE_SHDN};
	
	i2c_packet.address = TEMP_SENSOR_ADDRESS;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	
	// Configure the Micrel switch enable pin
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = SYSTEM_PINMUX_GPIO;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_DOWN;
	
	// Enable the sensor
	system_pinmux_pin_set_config(ADT7420_EN_PIN, &config_pinmux);
	port_pin_set_output_level(ADT7420_EN_PIN, true);
	
	// Write the packet to the sensor. If we timeout we break with no notification to the calling function
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
 		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Shut it back down, and initialize the global temperature array index
	port_pin_set_output_level(ADT7420_EN_PIN, false);
	ucTemperatureArrayPtr = 0;
}

/************************************************************************/
/*  @brief ADT7420_read_temp wakes up and reads the temperature sensor value once the value is read, it is stored in the global
/*  temperature value array for offload at a later time
/*  @param none
/*  @returns none                                                                     
/************************************************************************/

void ADT7420_read_temp(void)
{
	// Create an I2C master packet and populate it with the configuration register address and the oneshot mode value
	// This wakes the sensor up, does a conversion, and then the sensor automatically goes back into low-power mode until its turned off
	
	// The timer used throughout the reads/writes ensure that we don't get stuck in an infinite loop if the sensor is unresponsive.
	uint16_t uiTemperature=0;
	struct i2c_master_packet i2c_packet;
	uint16_t uiTimer = 0;
	uint8_t ucDataBuffer[1] = {0};
	uint8_t wr_buffer[2] = {0,0};
	
	i2c_packet.address = TEMP_SENSOR_ADDRESS;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.hs_master_code = 0;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	wr_buffer[0] = TEMP_SENSOR_CONFIG_ADDR;
	wr_buffer[1] = TEMP_SENSOR_CONFIG_OP_MODE_OS;
	
	// Turn on the sensor
	port_pin_set_output_level(ADT7420_EN_PIN, true);
	
	// Delay long enough for the chip to fully power up. This is a busy-wait and is not great for energy.
	for(int i = 0; i < 10000; i++);
	
	// Write the packet
	do{
		status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	
	// Wait for 240 ms for the sampling to complete
	for(int i = 0; i < 10000; i++);
	
	uiTimer = 0;
	
	// Set register pointer to upper sensor value byte and write it
	i2c_packet.data_length = 1;
	wr_buffer[0] = TEMP_SENSOR_TEMP_REG_MS_ADDR;
	do{
		status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	
	uiTimer = 0;
	
	// Read the response from the sensor and store it in the upper byte of the temperature variable
	i2c_packet.data_length = 1;
	i2c_packet.data = ucDataBuffer;
	do{
		status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	uiTemperature = ucDataBuffer[0] << 8;
	
	uiTimer = 0;
	
	// Set register pointer to lower byte and write it
	i2c_packet.data = wr_buffer;
	i2c_packet.data_length = 1;
	wr_buffer[0] = TEMP_SENSOR_TEMP_REG_LS_ADDR;
	do{
		status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	i2c_packet.data_length = 1;
	i2c_packet.data = ucDataBuffer;
	
	uiTimer = 0;
	
	// Read the response and store it in the lower byte of the temperature variable
	do{
		status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet);
		if(uiTimer++ == i2c_master_instance.buffer_timeout) break;
	}while(status != STATUS_OK);
	uiTemperature = uiTemperature | ucDataBuffer[0];
	
	// Put the sensor in shutdown and store the temperature in the array
	port_pin_set_output_level(ADT7420_EN_PIN, false);
	uiTemperatureArray[ucTemperatureArrayPtr++] = uiTemperature;
	
	// If the core temperature is below the set threshold then we are inactive, and we assume stationary mode
	//		if the core temperature is above the threshold then we are active, and we assume we are still in stationary mode
	//		otherwise, we do nothing (this case only occurs for thresholds that are not equal)
	// NOTE--Raw Temp*128 is the conversion for positive numbers, if the animal falls below freezing, things may get weird.
	if(128*uiTemperature <= ucInactivityTemperatureThreshold){
		
		/* Inactive Mode */
		ucActiveInactive_Mode = INACTIVE_MODE;
		ucMotion_State = STATIONARY_MODE;
		
		// Disable the accelerometer activity interrupt
		ADXL375_disable_interrupt(ADXL375_INT_SRC_ACTIVITY);
		
		// We have switched modes, so the temperature data now needs to be marked as a new dataset
		ucTemperatureDataSets++;
		cTemperatureDataSetPtr[ucTemperatureDataSets] = ucTemperatureArrayPtr;
		get_timestamp(ucTemperatureTimestamps[ucTemperatureDataSets]);
	}else if(128*uiTemperature > ucActivityTemperatureThreshold){
		
		/* Active Mode */
		ucActiveInactive_Mode = ACTIVE_MODE;
		ucMotion_State = STATIONARY_MODE;
		
		// Re-enable the activity interrupt -- we need to do this because we are exiting inactive mode where the activity interrupt is disabledf
		ADXL375_enable_interrupt(ADXL375_INT_SRC_ACTIVITY);
		
		// We have switched modes, so the temperature data now needs to be marked as a new dataset
		ucTemperatureDataSets++;
		cTemperatureDataSetPtr[ucTemperatureDataSets] = ucTemperatureArrayPtr;
		get_timestamp(ucTemperatureTimestamps[ucTemperatureDataSets]);
	}
}