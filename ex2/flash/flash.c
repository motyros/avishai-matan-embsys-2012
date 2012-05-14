#include "flash.h"
#include "common_defs.h"

#define FLASH_BASE_ADDR 0x150

//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
{
	union
	{
		uint32_t data;
		struct
		{
		        uint8_t SCIP		:1;
			uint8_t cycleDone	:1;
			uint32_t reserved	:30;
		}
	}SR;
	
	union
	{
		uint32_t data;
		struct
		{
			uint8_t SCGO		:1;
			uint8_t SME		:1;
			uint8_t reserved1	:6;
			uint8_t CMD		:8;
			uint8_t FDBC		:6;
			uint16_t reserved2	:10;
		}
	}CR;

	uint32_t FADDR;
	uint32_t FDATA00;
	uint32_t FDATA01;
	uint32_t FDATA02;
	uint32_t FDATA03;
	uint32_t FDATA04;
	uint32_t FDATA05;
	uint32_t FDATA06;
	uint32_t FDATA07;
	uint32_t FDATA08;
	uint32_t FDATA09;
	uint32_t FDATA0A;
	uint32_t FDATA0B;
	uint32_t FDATA0C;
	uint32_t FDATA0D;
	uint32_t FDATA0E;
	uint32_t FDATA0F;
	
} volatile FlashRegs;
#pragma pack()
