/**
 /* @file ADXL375.c
 /* @brief ADXL375 accelerometer driver functions
 **/ 

#include "HAL.h"
#include <asf.h>

/ 
void configure_ADXL375(void)
{
	// All buffers are separate for readability
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	// FIFO Stream mode (ring buffer) and 32 data points
	uint8_t wr_buffer1[2] = {ADXL375_FIFO_ADDR, ADXL375_FIFO_STREAM | 0x1F};
	
	uint8_t wr_buffer4[2] = {ADXL375_INT_MAP_ADDR, (~ADXL375_INT_MAP_ACTIVITY & ~ADXL375_INT_SRC_WATERMARK & ~ADXL375_INT_SRC_WATERMARK) & 0xFF};
	// Enable interrupts for activity
	uint8_t wr_buffer5[2] = {ADXL375_INT_EN_ADDR, ADXL375_INT_EN_ACTIVITY | ADXL375_INT_SRC_INACTIVITY};
	// Set the part to measure mode -- this consumes lots of power. Should be changed to sleep.
	// There seems to be an issue triggering an interrupt in sleep mode (it doesn't work).
	uint8_t wr_buffer6[2] = {ADXL375_POWER_CTL_ADDR, ADXL375_POWER_CTL_MEASUSRE | ADXL375_POWER_CTL_MEASUSRE};
	
	// Calibrate the sensor before configuring anything else
	ADXL375_calibrate();
	
	ADXL375_set_activity_thresh(5, true, true, true);
	ADXL375_set_inactivity_thresh(7, 255, true, true, true);
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer1;
	
	/* Write each packet one at a time, bad for efficiency, but good for readability */
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	i2c_packet.data = wr_buffer4;
	timeout = 0;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	i2c_packet.data = wr_buffer5;
	timeout = 0;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	i2c_packet.data = wr_buffer6;
	timeout = 0;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
		
	// Set up the interrupt pin
	struct system_pinmux_config config_pinmux;	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = PINMUX_PA16A_EIC_EXTINT0;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	
	system_pinmux_pin_set_config(ADXL375_INT_PIN, &config_pinmux);
	system_pinmux_pin_set_input_sample_mode(ADXL375_INT_PIN, SYSTEM_PINMUX_PIN_SAMPLE_CONTINUOUS);
	
	// Disable the EIC so we can write regs
	REG_EIC_CTRLA = 0;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x02);
	
	// Enable interrupts on EXTINT[0]
	REG_EIC_INTENSET |= 0x01;
	if(!(REG_EIC_INTENSET & 0x01)) return;
	
	// Turn filtering off and set detection for falling edge for EXTINT[0]
	REG_EIC_CONFIG0 &= ~0x8;
	REG_EIC_CONFIG0 |= 0x4;
	if(!(REG_EIC_CONFIG0 & 0x4) && (REG_EIC_CONFIG0 & 0x8)) return;
	
	// Enable asynchronous interrupts for EXTINT[0]
	REG_EIC_ASYNCH |= 0x00000001;
	if(!(REG_EIC_ASYNCH & 0x01)) return;
	
	// Enable the EIC
	REG_EIC_CTRLA = 0x02;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x02);
	if(!(REG_EIC_CTRLA & 0x02)) return;
	
	// register the callback function
	if(!(extint_register_callback(ADXL375_ISR_Handler, 0, EXTINT_CALLBACK_TYPE_DETECT) == STATUS_OK)) return;
	// Set the buffer index pointer to 0
	uiAccelerometerMatrixPtr = 0;
	ADXL375_inactive_interrupts = 0;
	
}

/**************************************************************************/
/* @brief ADXL375_ISR_Handler ADXL375 ISR handler function
/* When INT1 pin on the ADXL375 is triggered, this function is called.
/* @params none
/* @returns none
**************************************************************************/
void ADXL375_ISR_Handler(void)
{
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t buffer = ADXL375_INT_SRC_ADDR;
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 1;
	i2c_packet.data = &buffer;
	
	// Check the interrupt source value
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	// If the source of the interrupt was movement, handle it.
	if(buffer & ADXL375_INT_SRC_ACTIVITY){
		// If we are in winter mode, then switch to summer mode otherwise stay in winter mode
		ucCurrent_Mode = ucCurrent_Mode == WINTER_MODE ? SUMMER_MODE : WINTER_MODE;
		// Movement interrupt triggered, enter 12.5 Hz sampling mode
		ADXL375_begin_sampling();		
	}
	// If the source of the interrupt is from the the FIFO filling up
	if(buffer & ADXL375_INT_SRC_WATERMARK){
		// FIFO is full, read it out
		// We know that 32 points are in the FIFO since thats the size we set it to
		for(int i = 0; i < 32; i++){
			ADXL375_read_samples(ucAccelerometerMatrix + uiAccelerometerMatrixPtr++, 1);
		}
		// If we have taken 2 sets of samples, then stop (5.12 seconds worth of data)
		if(!(uiAccelerometerMatrixPtr % 2)) ADXL375_end_sampling();
	}
	// If the source of the interrupt was from inactivity
	if(buffer & ADXL375_INT_SRC_INACTIVITY){
		// If we are in summer mode, and we have had greater than 3 interrupts, then switch to winter mode
		// 3 Interrupts yields 12.75 minutes (255/60 is 4.25 minutes per interrupt--255 is the number of seconds to wait for inactivity)
		if(++ADXL375_inactive_interrupts >= 3){
			ucCurrent_Mode = ucCurrent_Mode == SUMMER_MODE ? WINTER_MODE : SUMMER_MODE;
		}
		// If we are in winter mode already, then disable the inactivity interrupt
		if(ucCurrent_Mode == WINTER_MODE){
			ADXL375_disable_interrupt(ADXL375_INT_EN_INACTIVITY);
		}
	}
}

/**************************************************************************/
/* @brief ADXL375_disable_interrupt disables an interrupt source on the ADXL375.
/* @params[in] interrupt_src the integer representation of the interrupt source
/* (See ADXL375 datasheet for sources)
/* @returns none
**************************************************************************/
void ADXL375_disable_interrupt(uint8_t interrupt_src)
{
	struct i2c_master_packet i2c_packet;
	uint8_t wr_buffer[2] = {ADXL375_INT_EN_ADDR, interrupt_src};
	uint16_t timeout = 0;
		
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 1;
	i2c_packet.data = wr_buffer;
	
	// Write the interrupt enable address for readback
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	timeout = 0;
	// Read back the value from the interrupt enable reg
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Reset the buffer with the disabled interrupt bit set low
	wr_buffer[1] = wr_buffer[0] & ~interrupt_src;
	wr_buffer[0] = ADXL375_INT_EN_ADDR;
	timeout = 0;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
}

/**************************************************************************/
/* @brief ADXL375_enable_interrupt enables an interrupt source on the ADXL375.
/* @params[in] interrupt_src the integer representation of the interrupt source
/* (See ADXL375 datasheet for sources)
/* @returns none
**************************************************************************/
void ADXL375_enable_interrupt(uint8_t interrupt_src)
{
	struct i2c_master_packet i2c_packet;
	uint8_t wr_buffer[2] = {ADXL375_INT_EN_ADDR, interrupt_src};
	uint16_t timeout = 0;
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 1;
	i2c_packet.data = wr_buffer;
	
	// Write the interrupt enable address for readback
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	timeout = 0;
	// Read back the value from the interrupt enable reg
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Reset the buffer with the enabled interrupt bit set high
	wr_buffer[1] = wr_buffer[0] | interrupt_src;
	wr_buffer[0] = ADXL375_INT_EN_ADDR;
	timeout = 0;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
}

/************************************************************************/
/* @brief ADXL375_calibrate ADXL375 calibration function
/* It is important that the device be set so that the part is in its final orientation
/* when this function is called. That ensures accurate calibration
/* @params none
/* @returns none                                                                   
/************************************************************************/
void ADXL375_calibrate(void)
{
	
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer[3] = {ADXL375_DATAX0, ADXL375_DATAY0, ADXL375_DATAZ0};
	uint8_t wr_buffer2 = ADXL375_FIFO_STATUS_ADDR;
	int8_t data[6];
	volatile int8_t avgs[3] = {0, 0, 0};
	int8_t wr_buffer3[2];
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 1;
	i2c_packet.data = &wr_buffer2;
	
	ADXL375_begin_sampling();
	// Wait for some samples
 	for(int i = 0; i < 65535; i++);
	ADXL375_end_sampling();
	// Make sure some time has passed so that the data can be moved from the fifo to the regs
	for(int i = 0; i < 65535; i++);
	
	// Write the fifo status reg address for readback
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Read back the value from the fifo status reg
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	i2c_packet.data_length = 3;
	i2c_packet.data = wr_buffer;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	i2c_packet.data_length = 3;
	i2c_packet.data = data;
	for(int i = 0; i < wr_buffer2; i++){
		while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
			if(timeout++ == i2c_master_instance.buffer_timeout) break;
		}
		avgs[0] += (data[0]);
		avgs[1] += (data[1]);
		avgs[2] += (data[2]);
	}
	avgs[0] /= wr_buffer2;
	avgs[1] /= wr_buffer2;
	avgs[2] /= wr_buffer2;
	
	
	i2c_packet.data_length = 2;
	i2c_packet.data = &wr_buffer3;
	
	wr_buffer3[0] = ADXL375_OFSX_ADDR;
	wr_buffer3[1] = avgs[0];
	
	// Write offset x
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Write offset y
	i2c_packet.data_length = 2;
	wr_buffer3[0] = ADXL375_OFSY_ADDR;
	wr_buffer3[1] = avgs[1];
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}

	// Write offset y
	wr_buffer3[0] = ADXL375_OFSZ_ADDR;
	wr_buffer3[1] = avgs[2];
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Write offset y
	i2c_packet.data_length = 1;
	wr_buffer3[0] = ADXL375_OFSY_ADDR;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
}

/************************************************************************/
/* @briefA DXL375_set_activity_thresh sets the activity interrupt threshold for the ADXL375
/* @param[in] threshold, threshold vale for the interrupt to trigger
/* @param[in] x, indicates whether the x axis should be considered for interrupts
/* @param[in] y, indicates whether the y axis should be considered for interrupts
/* @param[in] z, indicates whether the z axis should be considered for interrupts
/* @returns none
/************************************************************************/
void ADXL375_set_activity_thresh(int8_t threshold, bool x, bool y, bool z)
{
	
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer1[2] = {ADXL375_THRESH_ACT_ADDR, threshold};
	uint8_t wr_buffer2[2] = {ADXL375_ACT_INACT_CTL_ADDR, (x ? ADXL375_ACT_INACT_ACT_X_EN : 0x00) | (y ? ADXL375_ACT_INACT_ACT_Y_EN : 0x00) | (z ? ADXL375_ACT_INACT_ACT_Z_EN : 0x00)};
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer1;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Read out the act inact ctl reg contents, then OR them with the value that needs to be set to set up the activity
	// that way we don't stomp a previously set value
	i2c_packet.data_length = 1;
	i2c_packet.data = wr_buffer2;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	timeout = 0;
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	wr_buffer2[1] |= wr_buffer2[0];
	wr_buffer2[0] = ADXL375_ACT_INACT_CTL_ADDR;
	
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer2;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
}

/************************************************************************/
/* @briefA DXL375_set_inactivity_thresh sets the inactivity threshold
/* and duration before triggering an interrupt for the ADXL375
/* @param[in] threshold, threshold vale for the interrupt to trigger
/* @param[in] duration, time in seconds for the inactivity to be maintained before interrupting
/* @param[in] x, indicates whether the x axis should be considered for interrupts
/* @param[in] y, indicates whether the y axis should be considered for interrupts
/* @param[in] z, indicates whether the z axis should be considered for interrupts
/* @returns none
/************************************************************************/
void ADXL375_set_inactivity_thresh(int8_t threshold, uint8_t duration, bool x, bool y, bool z)
{
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer1[2] = {ADXL375_THRESH_INACT_ADDR, threshold};
	uint8_t wr_buffer2[2] = {ADXL375_TIME_INACT_ADDR, duration};
	uint8_t wr_buffer3[2] = {ADXL375_ACT_INACT_CTL_ADDR, (x ? ADXL375_ACT_INACT_INACT_X_EN : 0x00) | (y ? ADXL375_ACT_INACT_INACT_Y_EN : 0x00) | (z ? ADXL375_ACT_INACT_INACT_Z_EN : 0x00)};

	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer1;

	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}

	i2c_packet.data = wr_buffer2;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Read out the act inact ctl reg contents, then OR them with the value that needs to be set to set up the inactivity
	// that way we don't stomp a previously set value
	i2c_packet.data_length = 1;
	i2c_packet.data = wr_buffer3;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	timeout = 0;
	while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	wr_buffer3[1] |= wr_buffer3[0];
	wr_buffer3[0] = ADXL375_ACT_INACT_CTL_ADDR;
	
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer3;
	timeout = 0;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	

}

/************************************************************************/
/* @brief ADXL375_begin_sampling starts sampling on the ADXL375
/* @params none
/* @returns none
/************************************************************************/
void ADXL375_begin_sampling(void)
{
	
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	// Set the output data rate for 12.5 Hz (Closest to 10 available) and set for low power mode ("somewhat more noisy" -- datasheet).
	uint8_t wr_buffer[2] = {ADXL375_BW_RATE_ADDR, ADXL375_BW_RATE_LOW_PWR | 0x07};
	uint8_t wr_buffer2[2] = {ADXL375_POWER_CTL_ADDR, ADXL375_POWER_CTL_MEASUSRE};	
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	// Write the datarate buffer
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
	// Write the measurement mode buffer
	i2c_packet.data = wr_buffer2;
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
}

/************************************************************************/
/* @brief ADXL375_end_sampling ends sampling on the ADXL375 (enters standby mode)
/* @params none
/* @returns none
/************************************************************************/
void ADXL375_end_sampling(void)
{
	
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t wr_buffer[2] = {ADXL375_POWER_CTL_ADDR, ~ADXL375_POWER_CTL_MEASUSRE};	
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	i2c_packet.data_length = 2;
	i2c_packet.data = wr_buffer;
	
	while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
		if(timeout++ == i2c_master_instance.buffer_timeout) break;
	}
	
}

/************************************************************************/
/* @brief ADXL375_read_samples function to read out the samples from the ADXL375 FIFO
/* Only uses the lower byte of data. This results in up to 12g
/* @param[in,out] data, pointer to data array of 3*length
/* @param[in] length, number of bytes to read out per axis
/* @returns none
/************************************************************************/
void ADXL375_read_samples(uint8_t* data, uint8_t length)
{
	struct i2c_master_packet i2c_packet;
	uint16_t timeout = 0;
	uint8_t tempBuffer[6];
	uint8_t wr_buffer[6] = {ADXL375_DATAX0, ADXL375_DATAX1, ADXL375_DATAY0, ADXL375_DATAY1, ADXL375_DATAZ0, ADXL375_DATAZ1};
	
	i2c_packet.address = ADXL375_ADDR;
	i2c_packet.ten_bit_address = false;
	i2c_packet.high_speed = false;
	
	for(int i = 0; i < length; i++){
		// Write the registers that should be read out
		i2c_packet.data_length = 6;
		i2c_packet.data = wr_buffer;
		while((status = i2c_master_write_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
			if(timeout++ == i2c_master_instance.buffer_timeout) break;
		}
		// Perform the readout
		i2c_packet.data_length = 6;
		i2c_packet.data = tempBuffer;
		while((status = i2c_master_read_packet_wait(&i2c_master_instance, &i2c_packet)) != STATUS_OK){
			if(timeout++ == i2c_master_instance.buffer_timeout) break;
		}
		data[i]		= tempBuffer[1];
		data[i+1]	= tempBuffer[3];
		data[i+2]	= tempBuffer[5];
	}
}