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
	
	// Set a new alarm for the interval depending on what mode we are in
	// This method is according to the Atmel App note AT03266
	if(ucCurrent_Mode){
		// Winter mode
		alarm.time.hour += 1;
		alarm.time.hour %= 24;
	}else{
		// Summer mode
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
/* @brief offload_data moves buffered temperature and acceleration
/* data to off-chip memory
/* @params none
/* @returns none
/************************************************************************/
void offload_data(void)
{
	uint8_t header[3] = {ucTemperatureArrayPtr + 1, uiAccelerometerMatrixPtr + 1, (7 << NIBBLE(1)) & 0x0F};
	// Write the header
	for(int i = 0; i < 3; i++)
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
	
	// Write the actual data
	for(int i = 1; i < ucTemperatureArrayPtr; i++){
		// If we are at the end of the die, then switch die and reset the address pointer
		// If we hit the end of the second die, then we restart at the beginning of the first die
		if(S70FL01_address >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
		}
		// The temperature values are delta encoded to 8 bits
		S70FL01_verified_write((uiTemperatureArray[i] - uiTemperatureArray[i-1]) & 0xFF, S70FL01_active_die, S70FL01_address++);
	}
	for(uint16_t i = 1; i < uiAccelerometerMatrixPtr; i++){
		// If we are at the end of the die, then switch die and reset the address pointer
		// If we hit the end of the second die, then we restart at the beginning of the first die
		if(S70FL01_address + 3 >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
		}
		// Only keep the lower 8 bits of the accelerometer data (this is around 12g which is more than a fighter jet.)
		S70FL01_verified_write(ucAccelerometerMatrix[uiAccelerometerMatrixPtr] & 0xFF, S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucAccelerometerMatrix[uiAccelerometerMatrixPtr+1] & 0xFF, S70FL01_active_die, S70FL01_address++);
		S70FL01_verified_write(ucAccelerometerMatrix[uiAccelerometerMatrixPtr+2] & 0xFF, S70FL01_active_die, S70FL01_address++);
	}
}

/************************************************************************/
/* @brief offload_temperature_data moves data from the internal temperature 
/* data buffer to off chip-memory
/* @params none
/* @returns none
/************************************************************************/
void offload_temperature_data(void)
{
	uint16_t value;
	// If we are at the end of the die, then switch die and reset the address pointer
	// If we hit the end of the second die, then we restart at the beginning of the first die
	if(S70FL01_address >= S70FL01_MAX_ADDR){
		S70FL01_active_die++;
		S70FL01_active_die %= 2;
		S70FL01_address = 0;
	}
	// Write the first value in the array everything after this point will be delta encoded
	S70FL01_verified_write(uiTemperatureArray[0], S70FL01_active_die, S70FL01_address);
	for(int i = 1; i < ucTemperatureArrayPtr; i++)
	{	
		// Get the delta
		value = uiTemperatureArray[i] - uiTemperatureArray[i-1];
		// Force the data into 4 bits, hope we don't lose any information
		// Upper nibble is the temperature descriptor, lower nibble is the value
		value = (TEMPERATURE_DESCRIPTOR << NIBBLE(1) & 0xF0) & (value & 0xF);
		// Write the encoded and described element to memory
		S70FL01_verified_write(value, S70FL01_active_die, S70FL01_address);
	}
}

/************************************************************************/
/* @brief offload_accelerometer_data moves data from the internal accelerometer
/* data buffer to off chip-memory
/* @params none
/* @returns none
/************************************************************************/
void offload_accelerometer_data(void)
{
	for(int i = 0; i < ucAccelerometerMatrix; i++)
	{
		
		// Write the data in the array
		S70FL01_verified_write(uiTemperatureArray[0], S70FL01_active_die, S70FL01_address);
	}
}