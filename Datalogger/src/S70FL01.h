/************************************************************************/
/* @file s70fl01.h
/* @brief contains register addresses and prototype declarations
/************************************************************************/

#ifndef S70FL01_H_
#define S70FL01_H_

#include <asf.h>

/* S70FL01 Memory Defines */
#define S70FL01_WREN		0x06
#define S70FL01_WRDI		0x04
#define S70FL01_WRSR		0x01
#define S70FL01_RDSR		0x05
#define S70FL01_READ		0x03
#define S70FL01_RDID		0x9F
#define S70FL01_SE			0xD8
#define S70FL01_BE			0xC7
#define S70FL01_PP			0x02
#define S70FL01_DP			0xB9
#define S70FL01_RES			0xAB
#define S70FL01_EN			PIN_PA18
#define S70FL01_CS1			PIN_PA05
#define S70FL01_CS2			PIN_PA04
#define S70FL01_MAX_ADDR	(1<<29)

uint8_t configure_S70FL01(uint8_t die_cs, bool erase_chip);
uint8_t S70FL01_verified_write(uint8_t byte, uint8_t die, uint32_t address);
uint8_t S70FL01_read_byte(uint8_t *byte, uint8_t die, uint32_t address, uint8_t length);


#endif /* S70FL01_H_ */