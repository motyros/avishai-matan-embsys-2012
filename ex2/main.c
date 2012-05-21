#include "timer/timer.h"
#include "input_panel/input_panel.h"
#include "LCD/LCD.h"

int gCounter = 0;
uint32_t gA = 0;
uint32_t gB = 0;
uint32_t gC = 0;
uint32_t gflushed1 = 1,gflushed2 = 1;
void incCounter()
{
    gCounter++; 
}

button gButton;
void flushComplete()
{
	if(gflushed1)
	{
		char line[] = {'e','r','t','Q','!',' ','3','e','$','%','t',')'};
		lcd_set_row(5, false, line, 12);
		gflushed1=0;
	}
	else
	{
		if(gflushed2)
		{
			char line[] = {'d','o',' ','i','t','!'};
			lcd_set_row(17, false, line, 6);
			gflushed2=0;
		}
	}
}

void buttonPressed(button b)
{
    gButton = b;
}

void main()
{
    //timer0_register(1,true,incCounter);
    ip_init(buttonPressed);
    ip_enable();
    _enable();
    lcd_init(flushComplete);
    char line[] = {'S','o','m','e','_','t','h','i','n','g'};
    lcd_set_row(4, true, line, 10);
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
