#include "LCD.h"
#include "common_defs.h"
#include "our_common_defs.h"

#define LCD_DBUF_ADDR 0x1F0
#define LCD_DCMD_ADDR 0x1F1
#define LCD_DIER_ADDR 0x1F2
#define LCD_DICR_ADDR 0x1F3
#define BIT0 1
#define SELECTED_BIT 7
#define INIT_CHAR ' '


void (*gpFlushCompleteCB)(void) = NULL;

//pack the struct with no internal spaces
#pragma pack(1)
typedef union
{
	uint8_t data;
	struct
	{
			uint8_t charEncode	: 7;
			uint8_t selected	: 1;
	}byte;
}LcdCharacter;

//the data of the lcd
LcdCharacter gLcdData[LCD_LINE_LENGTH*LCD_NUM_LINES];

#pragma pack()



result_t lcd_init(void (*flush_complete_cb)(void))
{
	uint32_t i;
	_sr(0,LCD_DBUF_ADDR);
	_sr(0,LCD_DCMD_ADDR);
	_sr(0,LCD_DIER_ADDR);
	_sr(0,LCD_DICR_ADDR);

	if(!flush_complete_cb)
	{
        	return NULL_POINTER;
    }

	//Initialize the LCD screen data
	for(i=0 ; i<LCD_LINE_LENGTH*LCD_NUM_LINES ; ++i)
	{
		gLcdData[i].byte.charEncode = INIT_CHAR;
	}
	
	//save the cb
    gpFlushCompleteCB = flush_complete_cb;

    return OPERATION_SUCCESS;
}

result_t lcd_set_row(uint8_t row_number, bool selected, char const line[], uint8_t length)
{
	uint8_t i,bitSelected=0;
	if(!line)
	{
		return NULL_POINTER;
	}

	//note:row_number starts from 0
	if(length > LCD_LINE_LENGTH || row_number >= LCD_NUM_LINES)
	{
		return INVALID_ARGUMENTS;	
	}

	//check the DCMD reg
	if(_lr(LCD_DCMD_ADDR)&BIT0)
	{	
		return NOT_READY;
	}

	if(length == 0)
	{
		return OPERATION_SUCCESS;
	}

	if(selected)
	{
		bitSelected = 1;	
	}
	//note:row_number starts from 0
	//the offset of the correct row
	uint32_t rowBytesOffset = LCD_LINE_LENGTH * row_number;
	
	//write the data to the gLcdData array
	for(i=0 ; i<length ; ++i)
	{	
		gLcdData[rowBytesOffset+i].byte.charEncode = line[i];
		gLcdData[rowBytesOffset+i].byte.selected = bitSelected;
	}

	//write the LCD data address to the DBUF register
	_sr((uint32_t)&gLcdData,LCD_DBUF_ADDR);

	//enable the DMA completion interrupt
	_sr(BIT0,LCD_DIER_ADDR);

	//start writing to the LCD display
	_sr(BIT0,LCD_DCMD_ADDR);

	return OPERATION_SUCCESS;
}

_Interrupt1 void lcdDisplayISR()
{	
	//disable the DMA completion interrupt
	_sr(0,LCD_DIER_ADDR);
	_sr(BIT0,LCD_DICR_ADDR);

	//call the CB function
	gpFlushCompleteCB();
}



