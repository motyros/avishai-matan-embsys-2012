#include "LCD.h"
#include "common_defs.h"

#define LCD_BASE_ADDR 0x1F0

//pack the struct with no internal spaces
#pragma pack(1)
typedef struct
{
	uint32_t DBUF;

	union
	{
		uint32_t data;
		struct
		{
		        uint8_t DMAcopy		:1;
			uint32_t reserved	:31;
		}
	}DCMD;
	
	union
	{
		uint32_t data;
		struct
		{
			uint8_t EnableDMA	:1;
			uint32_t reserved	:31;
		}
	}DIER;

	union
	{
		uint32_t data;
		struct
		{
			uint8_t  DMAcycle	:1;
			uint32_t reserved	:31;
		}
	}DICR;

	
} volatile LCDRegs;
#pragma pack()
