/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include "HAL.h"

int main (void)
{
	system_init();
	system_interrupt_enable_global();
	
	
	/* Configure various sensors and their associated peripherals */
  	configure_i2c();
  	
 	configure_mag_sw_int(extint_callback);
  	configure_S70FL01(S70FL01_CS1, false);
  	configure_SP1ML();
  	configure_ADXL375(); 	
 	configure_sleepmode();
  	configure_rtc();
	port_pin_set_output_level(SP1ML_EN_PIN,true);
	configure_ADT7420();
	configure_databuffers();
	
	// Assume that we are in active mode during stationary period
	ucActiveInactive_Mode = ACTIVE_MODE;
	ucMotion_State = STATIONARY_MODE;
	
	// Per Dr. Buck, 30C is the pivot point -- These don't necessarily have to be the same, they can build in some hysteresis depending upon the subject
	ucActivityTemperatureThreshold = 30;
	ucInactivityTemperatureThreshold = 30;
	
	while(true)
	{	
		if((uiAccelerometerMatrixPtr > (300 - 32)) || (ucTemperatureArrayPtr > 71)){
			// Accelerometer total buffer size minus the ADXL375 internal FIFO size
			// If either buffer is full enough that another set of samples cannot be stored, trigger an offload
			offload_data();
			uiAccelerometerMatrixPtr = 0;
			ucTemperatureArrayPtr = 0;
		}
		// If we are at the end of the die, then switch die and reset the address pointer
		// If we hit the end of the second die, then we restart at the beginning of the first die (ring buffer)
		if(S70FL01_address >= S70FL01_MAX_ADDR){
			S70FL01_active_die++;
			S70FL01_active_die %= 2;
			S70FL01_address = 0;
		}
		ADT7420_read_temp();
		// Housekeeping done -- go back to sleep
		sleep();
	}
}

void extint_callback(void)
{
	while(true)
	{
		// do some stuff
	}
}
