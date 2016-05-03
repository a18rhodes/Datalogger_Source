/************************************************************************/
/* @file s70fl01.c
/* @brief contains driver functions for the S70FL01 memory module
/************************************************************************/

#include "HAL.h"
#include <asf.h>

/************************************************************************/
/* @brief configure_s70fl01 configures the memory module
/* @params[in] die_cs, the die that should be configured in the S70FL01
/* @params[in] erase_chip, whether or not to erase the die upon config
/* @returns 0 if failure 1 if success
/************************************************************************/
uint8_t configure_S70FL01(uint8_t die_cs, bool erase_chip)
{
	S70FL01_active_die = 0;
	S70FL01_address = 0;
	static uint8_t rxBuffer[20];
	struct spi_config config_spi_master;
	struct spi_slave_inst_config slave_dev_config;
	struct system_pinmux_config config_pinmux;
	system_pinmux_get_config_defaults(&config_pinmux);
	config_pinmux.mux_position = SYSTEM_PINMUX_GPIO;
	config_pinmux.direction = SYSTEM_PINMUX_PIN_DIR_OUTPUT;
	config_pinmux.input_pull = SYSTEM_PINMUX_PIN_PULL_DOWN;

	// Setup Enable -- Disable the chip while configuring.
	system_pinmux_pin_set_config(S70FL01_EN, &config_pinmux);
	port_pin_set_output_level(S70FL01_EN, false);

	// Setup CS1#
	system_pinmux_pin_set_config(S70FL01_CS1, &config_pinmux);
	port_pin_set_output_level(S70FL01_CS2, true);
	
	// Setup CS2#
	system_pinmux_pin_set_config(S70FL01_CS2, &config_pinmux);
	port_pin_set_output_level(S70FL01_CS2, true);
	
	spi_slave_inst_get_config_defaults(&slave_dev_config);
	slave_dev_config.ss_pin = die_cs;
	spi_attach_slave(&slave, &slave_dev_config);
	
	/* Configure, initialize and enable SERCOM SPI module */
	spi_get_config_defaults(&config_spi_master);
	config_spi_master.run_in_standby = false;
	config_spi_master.mode_specific.master.baudrate = 10000UL;
	config_spi_master.character_size = SPI_CHARACTER_SIZE_8BIT;
	config_spi_master.data_order = SPI_DATA_ORDER_MSB;
	config_spi_master.mode = SPI_MODE_MASTER;
	config_spi_master.receiver_enable = true;
	config_spi_master.master_slave_select_enable = false;
	config_spi_master.generator_source = GCLK_GENERATOR_1;
	/** SPI MUX combination F. DOPO: 0x1 => SCK=PAD3, MOSI=PAD2, DIPO: 0x1 => MISO=PAD1 */
	config_spi_master.mux_setting = SPI_SIGNAL_MUX_SETTING_F;
	/* Configure pad 0 as unused */
	config_spi_master.pinmux_pad0 = PINMUX_UNUSED;
	/* Configure pad 1 as data out */
	config_spi_master.pinmux_pad1 = PINMUX_PA09D_SERCOM2_PAD1;
	/* Configure pad 2 for data in */
	config_spi_master.pinmux_pad2 = PINMUX_PA10D_SERCOM2_PAD2;
	/* Configure pad 3 for SCK */
	config_spi_master.pinmux_pad3 = PINMUX_PA11D_SERCOM2_PAD3;
	
	spi_init(&spi_master_instance, SERCOM2, &config_spi_master);
	spi_enable(&spi_master_instance);
	spi_enabled = true;
	
	// Make sure our RXBuffer is empty
	for(int i = 0; i < 20; i++){
		rxBuffer[i] = 0;
	}
	
	// Enable the chip now that its configured
	port_pin_set_output_level(S70FL01_EN, true);
	for(int i = 0; i < 65535; i++);
	
	// Select chip
	port_pin_set_output_level(die_cs, false);
	for(int i = 0; i < 1000; i++);
	
	// Wait for the module to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Write the register
	while((status = spi_write(&spi_master_instance, S70FL01_RDID)) != STATUS_OK);
	// Read back the contents of the shift reg
	while(!spi_is_ready_to_read(&spi_master_instance));
	while((status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 20, 0xFF)) != STATUS_OK);
	
	// See if we got anything back. If not, then just return
	for(int i = 0; i < 20; i++){
		if(rxBuffer[i]){
			break;
			}else if(i == 19){
			port_pin_set_output_level(die_cs, true);
			port_pin_set_output_level(S70FL01_EN, false);
			return 0;
		}
	}
	rxBuffer[0] = 0;
	
	// We need to delay because the GPIO is faster than the serial out 100 is sufficient for 1 byte
	// Then give it a second to stabilize
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die_cs, true);
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die_cs, false);
	for(int i = 0; i < 100; i++);
	
	if(erase_chip){
		// Wait for the module to be ready
		while(!spi_is_ready_to_write(&spi_master_instance));
		// Set reg to erase entire array
		while(spi_write(&spi_master_instance, S70FL01_BE) != STATUS_OK);
		
		// Wait until the erase completes (Status Reg 0x01)
		// Wait for the module to be ready
		while(!spi_is_ready_to_write(&spi_master_instance));
		// Write the buffer to read back written data
		while(spi_write(&spi_master_instance, S70FL01_RDSR) != STATUS_OK);
		do{
			// Wait for the peripheral to be ready
			while(!spi_is_ready_to_read(&spi_master_instance));
			// Read back the contents of the shift reg
			while((status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 1, 0xFF)) != STATUS_OK);
		}while(rxBuffer[0] & 0x01);
	}
	// We need to delay because the GPIO is faster than the serial out 100 is sufficient for 1 byte => we have no idea how long this may take so max it out
	for(int i = 0; i < 65535; i++);
	port_pin_set_output_level(die_cs, true);
	port_pin_set_output_level(S70FL01_EN, false);
	spi_disable(&spi_master_instance);
	spi_enabled = false;
	S70FL01_active_die = S70FL01_CS1;
	S70FL01_address = 0;
	return 1;
	
}

/************************************************************************/
/* @brief s70fl01_verified_write writes a byte to the memory module
/* @params[in] byte data element to write
/* @params[in] die the die to write to in the memory module
/* @params[in] address the address to write to in the given die
/* @returns 0 if failure 1 if successful
/************************************************************************/
uint8_t S70FL01_verified_write(uint8_t byte, uint8_t die, uint32_t address)
{
	if(!spi_enabled)
	{
		spi_enable(&spi_master_instance);
		spi_enabled = true;
	}
	// Enable the chip
	port_pin_set_output_level(S70FL01_EN, true);
	for(int i = 0; i < 65535; i++);
	
	uint8_t rxBuffer[20];
	uint8_t wrBuffer[5] = {S70FL01_PP, (address>>16) & 0xFF, (address>>8) & 0xFF, (address>>0) & 0xFF, byte};
	uint8_t vfyBuffer[4] = {S70FL01_READ, (address>>16) & 0xFF, (address>>8) & 0xFF, (address>>0) & 0xFF};
		
	// Make sure our RXBuffer is empty
	for(int i = 0; i < 20; i++){
		rxBuffer[i] = 0;
	}
	
	// Select chip
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 1000; i++);
	
	// Wait for the module to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Set WREN so we can write to memory
	while(spi_write(&spi_master_instance, S70FL01_WREN) != STATUS_OK);
	
	// We need to delay because the GPIO is faster than the serial out
	// Then give it a second to stabilize
	for(int i = 0; i < 1000; i++);
	port_pin_set_output_level(die, true);
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 500; i++);
	
	// Wait for the peripheral to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Write the register
	while((status = spi_write(&spi_master_instance, S70FL01_RDSR)) != STATUS_OK);
	for(int i = 0; i < 1000; i++);
	// Wait for the peripheral to be ready
	while(!spi_is_ready_to_read(&spi_master_instance));
	// Read back the contents of the shift reg
	// For some reason, if the buffer length is set to 1 the readout does not occur
	while((status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 20, 0xFF)) != STATUS_OK);
	
	if(!(rxBuffer[0] & 0x02)){
		// WREN didn't work, so we don't need to waste time doing the rest of the operations.
	 	port_pin_set_output_level(die, true);
	 	port_pin_set_output_level(S70FL01_EN, false);
		return 0;
	}
	
	// We need to delay because the GPIO is faster than the serial out
	// Then give it a second to stabilize
	for(int i = 0; i < 1000; i++);
	port_pin_set_output_level(die, true);
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 500; i++);
	
	
	// Wait for the module to be ready -- maybe not necessary here
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Perform write operation
	while(spi_write_buffer_wait(&spi_master_instance, wrBuffer, 5) != STATUS_OK);
	
	// We need to delay because the GPIO is faster than the serial out
	// Then give it a second to stabilize
	for(int i = 0; i < 500; i++);
	port_pin_set_output_level(die, true);
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 500; i++);
	
	// Wait until the write completes (Status Reg 0x01)
	// Wait for the module to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Write the buffer to read back written data
	while(spi_write(&spi_master_instance, S70FL01_RDSR) != STATUS_OK);
	do{
		// Wait for the peripheral to be ready
		while(!spi_is_ready_to_read(&spi_master_instance));
		// Read back the contents of the shift reg
		status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 20, 0xFF);
	}while(rxBuffer[0] & 0x01);
	
	// We need to delay because the GPIO is faster than the serial out
	// Then give it a second to stabilize
	for(int i = 0; i < 65535; i++);
	port_pin_set_output_level(die, true);
	for(int i = 0; i < 100; i++);
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 500; i++);
	
	
	// Wait for the module to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Write the buffer to read back written data
	while(spi_write_buffer_wait(&spi_master_instance, vfyBuffer, 4) != STATUS_OK);
	// Read back the contents of the shift reg
	// Wait for the peripheral to be ready
	while(!spi_is_ready_to_read(&spi_master_instance));
	while((status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 20, 0xFF)) != STATUS_OK);
	// We need to delay because the GPIO is faster than the serial out 100 is sufficient for 1 byte => 2000 should be good
	for(int i = 0; i < 2000; i++);
	port_pin_set_output_level(die, true);
	port_pin_set_output_level(S70FL01_EN, false);
	spi_disable(&spi_master_instance);
	spi_enabled = false;
	return rxBuffer[0] == byte ? 1 : 0;
}

/************************************************************************/
/* @brief @s70fl01_read_byte reads a byte from the memory module
/* @params[in,out] byte pointer to a data value that is populated with the readout value
/* @params[in] die the die from which to read
/* @params[in] address the starting address to read from
/* @params[in] length the number of bytes to read
/************************************************************************/
uint8_t S70FL01_read_byte(uint8_t *byte, uint8_t die, uint32_t address, uint8_t length)
{
	if(!spi_enabled)
	{
		spi_enable(&spi_master_instance);
		spi_enabled = true;
	}
	// Power the chip, and wait for a bit
	port_pin_set_output_level(S70FL01_EN, true);
	
	uint8_t rxBuffer[20];
	uint8_t rdBuffer[4] = {S70FL01_READ, (address>>16) & 0xFF, (address>>8) & 0xFF, (address>>0) & 0xFF};
		
	for(int i = 0; i < 20; i++)
	{
		rxBuffer[i] = 0;
	}
	
	// Power the chip, and wait for a bit
	port_pin_set_output_level(S70FL01_EN, true);
	for(int i = 0; i < 1000; i++);
	
	// Give it a second to stabilize
	port_pin_set_output_level(die, false);
	for(int i = 0; i < 1000; i++);
	
	
	// Wait for the module to be ready
	while(!spi_is_ready_to_write(&spi_master_instance));
	// Write the buffer to read back written data
	while(spi_write_buffer_wait(&spi_master_instance, rdBuffer, 4) != STATUS_OK);
	// Read back the contents of the shift reg
	// Wait for the peripheral to be ready
	while(!spi_is_ready_to_read(&spi_master_instance));
	while((status = spi_read_buffer_wait(&spi_master_instance, rxBuffer, 20, 0xFF)) != STATUS_OK);
	
	for(int i = 0; i < length; i++){
		byte[i] = rxBuffer[i];	
	}
	// We need to delay because the GPIO is faster than the serial out 100 is sufficient for 1 byte => 2000 should be good
	for(int i = 0; i < 65535; i++);
	port_pin_set_output_level(die, true);
	port_pin_set_output_level(S70FL01_EN, false);
	spi_disable(&spi_master_instance);
	spi_enabled = false;
	return 1;
}