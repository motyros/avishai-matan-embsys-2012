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
    char line[] = {'3',' ','&','m','a'};
    lcd_set_row(4, true, line, 5);
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
