/************************************************************************/
/* @file HAL.c
/* @brief General driver functions for the SAM L21
/************************************************************************/
#include "HAL.h"
#include <asf.h>

/************************************************************************/
/* @brief sleep function to replace the general system_sleep function
/* This function is necessary to fix the errata for the part upon wakeup and sleep every time
/* @params none
/* @returns none
/************************************************************************/
void sleep(void)
{
	/* Errata 13901 fix */
	SUPC->VREF.reg |= (1 << 8);
	SUPC->VREG.bit.SEL = 0;
	/* Errata 14539 fix */
	GCLK->GENCTRL->bit.SRC = SYSTEM_CLOCK_SOURCE_ULP32K;
	// Make sure the DFLL and DPLL are shut off before entering sleep mode again
	system_clock_source_disable(SYSTEM_CLOCK_SOURCE_DFLL);
	system_clock_source_disable(SYSTEM_CLOCK_SOURCE_DPLL);
	// Put the part in sleep mode
	system_sleep();
}

/************************************************************************/
/* @brief configures the SAM L21's sleep mode to consume as little power
/* as possible
/* @params none
/* @returns none
/************************************************************************/
void configure_sleepmode(void)
{
	struct system_standby_config stby_config;
	system_standby_get_config_defaults(&stby_config);
	system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);

	// Enable dynamic power gating
	stby_config.enable_dpgpd0 = true;
	stby_config.enable_dpgpd1 = true;
	stby_config.power_domain = SYSTEM_POWER_DOMAIN_DEFAULT;

	/* Errata 13599:
	 * In Standby mode, when Power Domain 1 is power gated,
	 * devices can show higher consumption than expected.
	*/
	stby_config.power_domain = SYSTEM_POWER_DOMAIN_PD01;
	
	// Force buck mode on the internal regulator
	SUPC->VREG.bit.SEL = 1;
	SUPC->VREG.bit.RUNSTDBY = 1;
	SUPC->VREF.bit.ONDEMAND = 1;

	system_standby_set_config(&stby_config);

	system_flash_set_waitstates(1);
	
	/* BOD33 disabled */
	SUPC->BOD33.reg &= ~SUPC_BOD33_ENABLE;
}

/************************************************************************/
/* @brief function to configure the SAM L21 RTC
/* @params none
/* @returns none
/************************************************************************/
void configure_rtc(void)
{
	uint8_t ucIndex;
    /* Initialize RTC in calendar mode. */
    struct rtc_calendar_config config_rtc_calendar;
    rtc_calendar_get_config_defaults(&config_rtc_calendar);
	// Alarm time for first time based interrupt
    alarm.time.year = 2015;
    alarm.time.month = 11;
    alarm.time.day = 2;
    alarm.time.hour = 19;
    alarm.time.minute = 31;
    alarm.time.second = 00;
	// Configure 24 hour clock mode, set the alarm, and use a grainularity of seconds
    config_rtc_calendar.clock_24h = true;
    config_rtc_calendar.alarm[0].time = alarm.time;
    config_rtc_calendar.alarm[0].mask = RTC_CALENDAR_ALARM_MASK_SEC;
    rtc_calendar_init(&rtc_instance, RTC, &config_rtc_calendar);
    rtc_calendar_enable(&rtc_instance);
    /* Set current time. */
    struct rtc_calendar_time time;
    rtc_calendar_get_time_defaults(&time);
    time.year = 2015;
    time.month = 11;
    time.day = 2;
    time.hour = 19;
    time.minute = 31;
    time.second = 00;
    rtc_calendar_set_time(&rtc_instance, &time);
	
    struct rtc_calendar_events events;
    events.generate_event_on_overflow = false;
	// Disable all periodic alarms
    for(ucIndex = 0; ucIndex < 8; ucIndex++){
        events.generate_event_on_periodic[ucIndex] = false;
    }
	// Enable alarm 1
    events.generate_event_on_alarm[0] = true;
    for(ucIndex = 1; ucIndex < RTC_NUM_OF_ALARMS; ucIndex++){
        events.generate_event_on_alarm[ucIndex] = false;
    }
	// Register the callback function
    rtc_calendar_register_callback(&rtc_instance, rtc_match_callback, RTC_CALENDAR_CALLBACK_ALARM_0);
    rtc_calendar_enable_callback(&rtc_instance, RTC_CALENDAR_CALLBACK_ALARM_0);
}

/************************************************************************/
/* @brief function that gets called upon RTC driven wakeup
/* @params none
/* @returns none
/************************************************************************/
void rtc_match_callback(void)
{
	struct rtc_calendar_time stCurrent_time;
	/* Errata 13901 fix */
	SUPC->VREF.reg &= ~(1 << 8);
	SUPC->VREG.bit.SEL = 1;
	/* Errata 14539 fix */
	GCLK->GENCTRL->bit.SRC = SYSTEM_CLOCK_SOURCE_OSC16M;
	
	ADT7420_read_temp();
	
	// Set a new alarm for the interval depending on what mode we are in
	// This method is according to the Atmel App note AT03266
	if(ucActiveInactive_Mode == INACTIVE_MODE){
		// Inactive mode
		alarm.time.hour += 2;
		alarm.time.hour %= 24;
	}else{
		// Active mode
		alarm.time.minute += 20;
		alarm.time.minute %= 60;
	}
}

/************************************************************************/
/* @brief I2C configuration function for SAM L21
/* @params none
/* @returns none
/************************************************************************/
void configure_i2c(void)
{	
	i2c_master_get_config_defaults(&config_i2c_master);
	/* Change buffer timeout to something longer */
	config_i2c_master.buffer_timeout = 65535;
	config_i2c_master.run_in_standby = true;
	config_i2c_master.baud_rate = I2C_MASTER_BAUD_RATE_100KHZ;
	config_i2c_master.pinmux_pad0 =  PINMUX_PA22C_SERCOM3_PAD0;
	config_i2c_master.pinmux_pad1 =  PINMUX_PA23C_SERCOM3_PAD1;
	
	/* Initialize and enable device with config */
	i2c_master_init(&i2c_master_instance, SERCOM3, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	
}

/************************************************************************/
/* @brief magnetic switch interrupt configuration
/* @params [in] callback, pointer to function that gets called when the magnetic switch is triggered
/* @returns none
/************************************************************************/
void configure_mag_sw_int(void (*callback)(void))
{

	struct system_pinmux_config config_pinmux;	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = PINMUX_PA17A_EIC_EXTINT1;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_INPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_NONE;
	
	system_pinmux_pin_set_config(PIN_PA17, &config_pinmux);
	system_pinmux_pin_set_input_sample_mode(PIN_PA17, SYSTEM_PINMUX_PIN_SAMPLE_CONTINUOUS);
	
	// Disable the EIC so we can write regs
	REG_EIC_CTRLA = 0;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x02);
	
	// Enable interrupts on EXTINT[1]
	REG_EIC_INTENSET |= 0x02;
	if(!(REG_EIC_INTENSET & 0x02)) return;
	
	// Turn filtering off and set detection for falling edge for EXTINT[1]
	REG_EIC_CONFIG0 &= ~0x80;
	REG_EIC_CONFIG0 |= 0x20;
	if(!(REG_EIC_CONFIG0 & 0x20) && (REG_EIC_CONFIG0 & 0x80)) return;
	
	// Enable asynchronous interrupts for EXTINT[1]
	REG_EIC_ASYNCH |= 0x00000002;
	if(!(REG_EIC_ASYNCH & 0x02)) return;
	
	// Enable the EIC
	REG_EIC_CTRLA = 0x02;
	// Wait for the sync to complete
	while(REG_EIC_SYNCBUSY & 0x02);
	if(!(REG_EIC_CTRLA & 0x02)) return;
	
	// Register the callback
	if(!(extint_register_callback(callback, 1, EXTINT_CALLBACK_TYPE_DETECT) == STATUS_OK)) return;

}

/************************************************************************/
/* @brief Initializes the accelerometer and temperature data buffers
/* Only call at the start of execution, otherwise data will be lost
/* @params none
/* @returns none
/************************************************************************/
void configure_databuffers(void)
{
	// Configure the buffers and their pointers
	ucTemperatureArrayPtr = 0;
	uiAccelerometerMatrixPtr = 0;
	ucTemperatureDataSets = 0;
	uiAccelerometerDataSets = 0;
	// Set the first element of the data set pointer vector to be 0 (the 0th element is where the first data set starts)
	cTemperatureDataSetPtr[ucTemperatureDataSets] = ucTemperatureArrayPtr;
	iAccelerometerDataSetPtr[uiAccelerometerDataSets] = ucAccelerometerMatrix;
	// Set all the rest of the elements of the data set pointer vectors to be -1 (-1 represents unused)
	for(int i = 1; i < TEMP_BUFFER_SIZE; i++){
		cTemperatureDataSetPtr[i] = -1;
	}
	for(int i = 1; i < ACCEL_BUFFER_SIZE; i++){
		iAccelerometerDataSetPtr[i] = -1;
	}
}

/************************************************************************/
/* @brief get_timestamp get the system timestamp in seconds since 2000
/* @params ucTimestampVector vector that will contain the timestamp
/* @returns none
/************************************************************************/
void get_timestamp(uint8_t * ucTimestampVector)
{
	uint32_t ulRegVal;
	struct rtc_calendar_time stCurrentTime;
	rtc_calendar_get_time(&rtc_instance, &stCurrentTime);
	ulRegVal = rtc_calendar_time_to_register_value(&rtc_instance, &stCurrentTime);
	
	*(ucTimestampVector + 3) = ulRegVal >> 0  & 0xFF;
	*(ucTimestampVector + 2) = ulRegVal >> 8  & 0xFF;
	*(ucTimestampVector + 1) = ulRegVal >> 16 & 0xFF;
	*(ucTimestampVector + 0) = ulRegVal >> 24 & 0xFF;
	
}

/************************************************************************/
/* @brief offload_data moves buffered temperature and acceleration
/* data to off-chip memory
/* @params none
/* @returns none
/************************************************************************/
void offload_data(void)
{	
	cpu_irq_enter_critical();
	uint8_t ucHeaderSize = 2 + 4*(ucTemperatureDataSets + uiAccelerometerDataSets);
	uint8_t header[ucHeaderSize];
	uint8_t ucHeaderIndex = 0, ucDataIndex = 0;
	header[ucHeaderIndex++] = ucHeaderSize;
	header[ucHeaderIndex++] = ucTemperatureDataSets + uiAccelerometerDataSets;
	// Using these for loops is a bit overkill, but it shows more clearly that the upper nibble and lower nibble are
	//		separate fields.
	// Fill in the descriptors the temperature data sets
	for(ucDataIndex = 0; ucDataIndex < ucTemperatureDataSets - 1; ucDataIndex+=2){
		header[ucHeaderIndex++] = LNIBBLE(TEMPERATURE_DESCRIPTOR) | UNIBBLE(TEMPERATURE_DESCRIPTOR << 4);
	}
	// If there are an uneven number of temperature data sets, then we need to put one temp descriptor with one accel descriptor
	ucDataIndex = 0;
	if(ucTemperatureDataSets % 2){
		header[ucHeaderIndex++] = LNIBBLE(TEMPERATURE_DESCRIPTOR) | UNIBBLE(ACCEL_DESCRIPTOR << 4);
		ucDataIndex = 1;
	}
	// Fill in the accel descriptors
	for(ucDataIndex; ucDataIndex < uiAccelerometerDataSets - 1; ucDataIndex+=2){
		header[ucHeaderIndex++] = LNIBBLE(ACCEL_DESCRIPTOR) | UNIBBLE(ACCEL_DESCRIPTOR << 4);
	}
	// If there is an uneven number of datasets for accel, then we need to put the lower nibble in as an accel descriptor
	if(ucTemperatureDataSets % 2){
		header[ucHeaderIndex++] = LNIBBLE(ACCEL_DESCRIPTOR);
	}
	// Fill in the data set lengths
	for(ucDataIndex = 0; ucDataIndex < ucTemperatureDataSets - 1; ucDataIndex++){
		header[ucHeaderIndex++] = cTemperatureDataSetPtr[ucHeaderIndex + 1] - cTemperatureDataSetPtr[ucHeaderIndex];
	}
	// Fill in the data set lengths
	for(ucDataIndex = 0; ucDataIndex < uiAccelerometerDataSets - 1; ucDataIndex++){
		header[ucHeaderIndex++] = iAccelerometerDataSetPtr[ucHeaderIndex + 1] - iAccelerometerDataSetPtr[ucHeaderIndex];
	}
	// Fill in the data lengths, we are assuming 8 bits each
	for(ucDataIndex = 0; ucDataIndex < ucTemperatureDataSets - 1; ucDataIndex++){
		header[ucHeaderIndex++] = 8;
	}
	for(ucDataIndex = 0; ucDataIndex < uiAccelerometerDataSets - 1; ucDataIndex++){
		header[ucHeaderIndex++] = 8;
	}
	// Write the header
	for(int i = 0; i < ucHeaderSize; i++)
	{
		// If we are at the end of the die, then switch die and reset the address pointer
		// If we hit the end of the second die, then we restart at the beginning of the first dei
		if(S70FL01_address >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
	 	}
		S70FL01_verified_write(header[i], S70FL01_active_die, S70FL01_address++);
	}
	// TODO change to the new format
	
	// Loop over all the data sets in the temperature data
	for(int i = 0, ucDataIndex = 0; i < ucTemperatureDataSets - 1; i++){
		// Check that the memory is not full
		if(S70FL01_address >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
		}
		// We have reached the end of actual data
		if(cTemperatureDataSetPtr[i] < 0){
			break;
		}
		// Write the timestamps
		S70FL01_verified_write(ucTemperatureTimestamps[i+0], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucTemperatureTimestamps[i+1], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucTemperatureTimestamps[i+2], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucTemperatureTimestamps[i+3], S70FL01_active_die, S70FL01_address++);
		// Iterate over each block of data
		for(int j = cTemperatureDataSetPtr[i]; j < cTemperatureDataSetPtr[i] - cTemperatureDataSetPtr[i+1]; j++){
			// Check that the memory is not full
			if(S70FL01_address >= S70FL01_MAX_ADDR){
				S70FL01_active_die++;
				S70FL01_active_die %= 2;
				S70FL01_address = 0;
			}
			// If we are at the first point, write the entire data point
			if(j == 0 || j == cTemperatureDataSetPtr[i]){
				S70FL01_verified_write(uiTemperatureArray[j] >> 0x00 & 0xFF, S70FL01_active_die, S70FL01_address++);
				S70FL01_verified_write(uiTemperatureArray[j] >> 0x08 & 0xFF, S70FL01_active_die, S70FL01_address++);
			}
			// Otherwise delta encode and write
			else{
				S70FL01_verified_write((uiTemperatureArray[j] - uiTemperatureArray[j-1]) >> 0x08 & 0xFF, S70FL01_active_die, S70FL01_address++);
			}
		}
	}
	// Loop over all the data sets in the accel data
	for(int i = 0, ucDataIndex = 0; i < uiAccelerometerDataSets - 1; i++){
		// Check that the memory is not full
		if(S70FL01_address >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
		}
		if(iAccelerometerDataSetPtr[i] < 0){
			break;
		}
		S70FL01_verified_write(ucAccelTimestamps[i+0], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucAccelTimestamps[i+1], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucAccelTimestamps[i+2], S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucAccelTimestamps[i+3], S70FL01_active_die, S70FL01_address++);
		// Iterate over each block of data
		for(int j = iAccelerometerDataSetPtr[i]; j < iAccelerometerDataSetPtr[i] - iAccelerometerDataSetPtr[i+1]; j++){
			// Check that the memory is not full
			if(S70FL01_address >= S70FL01_MAX_ADDR){
				S70FL01_active_die++;
				S70FL01_active_die %= 2;
				S70FL01_address = 0;
			}
			// We are already only storing 8 bit numbers for the accel data, so no need for delta encoding
			S70FL01_verified_write(ucAccelerometerMatrix[j+0], S70FL01_active_die, S70FL01_address++);
			S70FL01_verified_write(ucAccelerometerMatrix[j+1], S70FL01_active_die, S70FL01_address++);
			S70FL01_verified_write(ucAccelerometerMatrix[j+2], S70FL01_active_die, S70FL01_address++);			
		}
	}
	// Reconfigure the buffers and their pointers
	configure_databuffers();
	cpu_irq_leave_critical();	
}