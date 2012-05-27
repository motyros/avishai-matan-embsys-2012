#include "timer/timer.h"
#include "input_panel/input_panel.h"
#include "flash/flash.h"
#include "LCD/LCD.h"

int gCounter = 0;
uint32_t gA = 0;
uint32_t gB = 0;
uint32_t gC = 0;
void incCounter()
{
    gCounter++; 
}

bool gFlashOpDone = false;

button gButton;
void flushComplete()
{
    //gFlashOpDone = true;
}

void buttonPressed(button b)
{
    gButton = b;
}



void flashOpComplete(void)
{
    gFlashOpDone = true;

}

void flashReadDone(uint8_t const *buffer, uint32_t size)
{
    gFlashOpDone = true;
}

#define BUF_SIZE 70

result_t testFlash()
{
	uint8_t buf[BUF_SIZE];// = {'1','2','1','2','1','2','1','2','1','2'};
	uint8_t buf2[BUF_SIZE];

	uint32_t i = 0;
	result_t res;

	for (i = 0 ; i < BUF_SIZE-1 ; ++i)
	{
		buf[i] = 't';
	}
	buf[BUF_SIZE-1] = 'z';

	flash_init(flashReadDone,flashOpComplete);
        gFlashOpDone = false;
	res = flash_write_start(9,BUF_SIZE,(uint8_t*)&buf);
	while(!gFlashOpDone);
        gFlashOpDone = false;
	res = flash_read(9,BUF_SIZE,(uint8_t*)&buf2);
        while(!gFlashOpDone);
	res = flash_read(9,BUF_SIZE,(uint8_t*)&buf2);

	return res;
}

void main()
{
    //timer0_register(1,true,incCounter);
    //ip_init(buttonPressed);
    //ip_enable();
    _enable();
    testFlash();
    lcd_init(flushComplete);
    
    result_t result = NOT_READY;
    char line[] = {'S','o','m','e','_','t','h','i','n','g'};
    while (result == NOT_READY)
    {
        result  = lcd_set_row(4, true, line, 10);
    }
    
    result = NOT_READY;
    char line1[] = {'e','r','t','Q','!',' ','3','e','$','%','t',')'};
    while (result == NOT_READY)
    {
        result  =   lcd_set_row(5, false, line1, 12);
    }
    
    result = NOT_READY;
    char line2[] = {'d','o',' ','i','t','!'};
    while (result == NOT_READY)
    {
        result  = lcd_set_row(17, false, line2, 6);
    }
          
    while(true)
    {
        //counter
        gA = _lr(0x21);
        //control
        gB = _lr(0x22);
        //limit
        gC = _lr(0x23);
        gA = gA+1;
    }
}
