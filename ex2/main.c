#include "timer/timer.h"
int gCounter = 0;
uint32_t gA = 0;
uint32_t gB = 0;
uint32_t gC = 0;

void incCounter()
{
    gCounter++; 
}
void main()
{
    timer0_register(1,true,incCounter);
    _enable();
    while(true)
    {
        //counter
        gA = _lr(0x21);
        //control
        gB = _lr(0x22);
        //limit
        gC = _lr(0x23);
    }
}
