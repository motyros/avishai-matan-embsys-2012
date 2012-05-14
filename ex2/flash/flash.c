#include "flash.h"
#include "defs.h"

#define FLASH_BASE_ADDR 0x150

//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
{
	union
	{
		uint32_t data;
	        uint8_t scip		:1;
		uint8_t cycleDoneStatus 	:1;
		uint32_t reserved		:30;
	}SR;
	
	union
	{
		uint32_t data;
		uint8_t scgo		:1;
		uint8_t sme		:1;
		uint8_t reserved1		:6;
		uint8_t cmd		:8;
		uint8_t fdbc		:6;
		uint16_t reserved2	:10;
	}CR;

	uint32_t FADDR;
	uint32_t FDATA00;
	uint32_t FDATA01...FDATA0F
	
} volatile FlashRegs;
#pragma pack()
