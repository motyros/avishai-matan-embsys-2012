#include "timer/timer.h"
#include "input_panel/input_panel.h"
#include "LCD/LCD.h"

int gCounter = 0;
uint32_t gA = 0;
uint32_t gB = 0;
uint32_t gC = 0;
void incCounter()
{
    gCounter++; 
}

button gButton;
void flushComplete()
{
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
